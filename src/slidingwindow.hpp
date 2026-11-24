#pragma once

#include "misragries.hpp"

#include <map>
#include <deque>


constexpr int64_t WINDOW_SIZE = 7889238LL;  // 3 months: Sliding WIndow length
constexpr int64_t BUCKET_SIZE = 604800LL;   // 1 week: Bucket length
constexpr int B = 3;                        // at most B + 1 buckets per level

// Stream sections, time-based 
struct Bucket{
    MisraGries mg;                          // corresponding Misra Gries sketch
    int64_t oldest_timestamp;               // timestamp of oldest record stored in Bucket
    int64_t newest_timestamp;               // timestamp of newest record stored in Bucket
    int64_t element_count;                  // number of elements in Bucket
    int level;                              // highest merge level in Bucket

    Bucket(int s, int level = 0) 
        : mg(s), oldest_timestamp(0), newest_timestamp(0), element_count(0), level(level) {}
};

// Separate sketches for historical baseline and current activity
struct BurstSketches{
    MisraGries baseline;                    // all Buckets minus the most recent one
    MisraGries recent;                      // most recent Bucket in queue

    BurstSketches(size_t s): baseline(s), recent(s) {}
};

// Time-based Sliding Window using Misra Gries and the DGIM algorithm
// to track heavy hitters and detect bursts 
class SlidingWindow{
    public:
        SlidingWindow(size_t s) : s(s), current(s) {}
        void process(int64_t hash, int64_t timestamp);          // process new stream element
        // query() is not used anymore: I keep it to show the case where detection is done without baseline comparisons
        std::unordered_map<int64_t, int64_t> query();           // merge all Bucket's sketches in WINDOW_SIZE, estimate frequencies
        
        // diagnostics: Bucket count, total counters in Bucket queue, sealed Buckets
        size_t num_buckets() const { return buckets.size(); }   
        size_t total_counters() const {
            size_t t = current.mg.counters();
            for(const auto& b : buckets) {
                t += b.mg.counters();
            } return t;
        }
        int64_t sealed_counters() const { return sealed; }

        BurstSketches split_query();                            // partition baseline and recent activity for burst detection
        std::unordered_map<int64_t, double> burst_scores();     // compute burst scores

    private:
        bool merge();                                           // merge two oldest buckets in level if there are mroe than B+!
        void cascade_merge();                                   // cascading merge() until B+1 bound is held
        
        size_t s;                                               // matching with MisraGries parameter
        int64_t sealed = 0;                                     // sealed Buckets in queue
        std::deque<Bucket> buckets;                             // Buckets queue
        Bucket current;                                         // current Bucket accumulating items in active BUCKET_SIZE
};