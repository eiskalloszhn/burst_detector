// Burst detector over r/memes subreddit (submissions + comments).
//
// Per record:
//   Merger (two time-sorted .zst files -> one stream in time order)
//     -> tokenize -> rolling_hash (5-gram fingerprints)
//     -> SlidingWindow (DGIM weekly buckets, one Misra-Gries sketch each)
//
// Every time a weekly bucket is sealed, phrases are scored based on recent vs baseline
// frequency, and phrases whose score is high are flagged:
// during the next bucket their distinct authors are counted with Bottom-k. A flagged
// phrase that is still bursting in the next bucket, with enough distinct authors,
// is written to the results .csv.

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <sys/resource.h>

#include "merge.hpp"
#include "tokenizer.hpp"
#include "rolling_hash.hpp"
#include "bottomk.hpp"

using clk = std::chrono::steady_clock;

// Configuration
namespace cfg {
    constexpr size_t SKETCH_SIZE    = 70000;        // Misra Gries counters per bucket (s)
    constexpr double MIN_SCORE      = 2.0;          // burst score needed to flag a phrase
    constexpr double MIN_AUTHORS    = 8;            // distinct authors needed to confirm a burst
    constexpr double MIN_RATIO      = 0.05;         // distinct authors / occurrences (bot filtering)
    constexpr int    PROGRESS_EVERY = 100000;       // records between progress lines / stats rows
}

// Elapsed seconds between two time points
static double sec(clk::time_point a, clk::time_point b) {
    return std::chrono::duration<double>(b - a).count();
}

// Peak query RAM usage in MiB
static double peak_rss_MiB() {
    rusage ru;
    getrusage(RUSAGE_SELF, &ru);
#ifdef __APPLE__
    return ru.ru_maxrss / (1024.0 * 1024.0);   // bytes on macOS
#else
    return ru.ru_maxrss / 1024.0;              // KB on Linux
#endif
}

// Keep only the phrases in "keep" set, drop all other phrase for memory bounding
static void prune(ReverseMap& reverse_map, const std::unordered_set<int64_t>& keep) {
    ReverseMap kept;
    for (int64_t hash : keep) {
        auto it = reverse_map.find(hash);
       if (it != reverse_map.end()) kept.emplace(hash, std::move(it->second));
       }
    reverse_map = std::move(kept);
}

// Statistics: print to .csv sketch size, execution times
struct Stats {
    std::ofstream out;
    clk::time_point start = clk::now();
    clk::time_point prev  = start;
    int64_t prev_records = 0;
    int64_t tokens = 0;
    double t_read = 0, t_tok = 0, t_hash = 0, t_query = 0;

    explicit Stats(const std::string& path) : out(path) {
        out << "records,elapsed_s,interval_rec_per_s,peak_rss_mb,buckets,mg_counters,"
               "reverse_map,known_hitters,bottomk_sketches,query_ms,total_tokens,"
               "t_read_s,t_tok_s,t_hash_s,t_query_s\n";
    }

    void row(int64_t records, const SlidingWindow& sw, size_t reverse_map_size,
             size_t known, size_t sketches, double query_ms) {
        auto now = clk::now();
        double dt = sec(prev, now);
        out << records << ',' << sec(start, now) << ','
            << (dt > 0 ? (records - prev_records) / dt : 0.0) << ','
            << peak_rss_MiB() << ','
            << sw.num_buckets() << ',' << sw.total_counters() << ','
            << reverse_map_size << ',' << known << ',' << sketches << ','
            << query_ms << ',' << tokens << ','
            << t_read << ',' << t_tok << ',' << t_hash << ',' << t_query << '\n';
        out.flush();
        prev = now;
        prev_records = records;
    }
};

// Burst detection
// Called once a weekly bucket has been sealed:
//  – phrases flagged at the previous bucket are written to the .csv if they are
//     still bursting and their distinct authors (counted during this bucket) 
//     pass the MIN_AUTHORS and MIN_RATIO check
//  – every phrase bursting now starts collecting its authors for the next check
static void process_bucket(SlidingWindow& sw,
                           ReverseMap& reverse_map,
                           std::unordered_set<int64_t>& known_hitters,
                           std::unordered_map<int64_t, BottomK>& bottomk_map,
                           std::ofstream& csv, int bucket_no, int64_t last_ts) {
    
    std::unordered_map<int64_t, double> scores = sw.burst_scores();

    // Check previous week's (Bucket's) candidates
    for (int64_t hash : known_hitters) {
        auto sc = scores.find(hash);
        if (sc == scores.end() || sc->second < cfg::MIN_SCORE) continue;   // burst is over
        
        auto bk = bottomk_map.find(hash);
        auto rm = reverse_map.find(hash);
        if (bk == bottomk_map.end() || rm == reverse_map.end()) continue;  // phrase not seen again

        double authors = bk->second.estimate();
        double occ = (double) bk->second.occurrences();
        double ratio = occ > 0 ? std::min(1.0, authors / occ) : 0.0;
        if (authors < cfg::MIN_AUTHORS || ratio < cfg::MIN_RATIO) continue;  // filter spam/bot-like trends

        std::cout << "phrase: " << rm->second.first
                  << " | score: " << sc->second
                  << " | authors~" << authors
                  << " | ratio: " << ratio
                  << " | link: " << rm->second.second << std::endl;
        csv << bucket_no << ',' << last_ts << ','
            << rm->second.first << ','
            << sc->second << ',' << authors << ',' << ratio << ','
            << rm->second.second << '\n';
    }
    csv.flush();

    // Flag currently bursting phrases for next week monitoring
    std::unordered_set<int64_t> flagged;
    for (const auto& [hash, score] : scores)
        if (score >= cfg::MIN_SCORE) flagged.insert(hash);

    known_hitters = std::move(flagged);
    bottomk_map.clear();                       // author sketches restart for the new flagged set
    prune(reverse_map, known_hitters);         // prune unflagged phrases
}

// main
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "usage: " << argv[0]
                  << " <submissions.zst> <comments.zst> [results.csv] [stats.csv]\n";
        return 1;
    }

    Merger merger({argv[1], argv[2]});
    SlidingWindow sw(cfg::SKETCH_SIZE);
    ReverseMap reverse_map;
    std::unordered_set<int64_t> known_hitters;
    std::unordered_map<int64_t, BottomK> bottomk_map;

    std::ofstream csv(argc > 3 ? argv[3] : "bursts.csv");
    csv << "bucket,last_timestamp,phrase,score,authors_est,ratio,link\n";
    Stats stats(argc > 4 ? argv[4] : "stats.csv");

    int64_t records = 0, last_ts = 0, seen_sealed = 0;
    int bucket_no = 0;
    double query_ms = 0;

    try {
        while (true) {
            // Read and merge Records chronologically
            auto t0 = clk::now();
            std::optional<Record> record = merger.merged_next();
            auto t1 = clk::now();
            if (!record) break;

            // Clean text, tokenize
            last_ts = record->timestamp;
            std::vector<std::string> tokens = tokenize(record->text);
            auto t2 = clk::now();

            // Compute 5-gram Karp-Rabin hash and update sketches
            rolling_hash(sw, tokens, record->timestamp, record->link, record->author,
                         reverse_map, known_hitters, bottomk_map);
            auto t3 = clk::now();

            stats.t_read += sec(t0, t1);
            stats.t_tok  += sec(t1, t2);
            stats.t_hash += sec(t2, t3);
            stats.tokens += tokens.size();
            ++records;

            // Trigger burst detection when a new Bucket is sealed
            if (sw.sealed_counters() != seen_sealed) {
                seen_sealed = sw.sealed_counters();
                auto t4 = clk::now();
                process_bucket(sw, reverse_map, known_hitters, bottomk_map,
                               csv, ++bucket_no, last_ts);
                auto tq = sec(t4, clk::now());
                query_ms = tq * 1000.0;
                stats.t_query += tq;
            }

            // Log statistics
            if (records % cfg::PROGRESS_EVERY == 0) {
                std::cout << "processed: " << records << std::endl;
                stats.row(records, sw, reverse_map.size(), known_hitters.size(),
                          bottomk_map.size(), query_ms);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "exception at record " << records << ": " << e.what() << std::endl;
    }

    // Final summary row to .csv and terminal
    stats.row(records, sw, reverse_map.size(), known_hitters.size(),
              bottomk_map.size(), query_ms);
    std::cout << "TOTAL records=" << records
              << " time_s=" << sec(stats.start, clk::now())
              << " peak_rss_mb=" << peak_rss_MiB() << std::endl;
    return 0;
}