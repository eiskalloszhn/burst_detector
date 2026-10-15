#include <iostream>
#include <optional>
#include <chrono>
#include <unordered_map>
#include <unordered_set>

#include "json.hpp"
#include "reader.hpp"
#include "parser.hpp"
#include "merge.hpp"
#include "tokenizer.hpp"
#include "rolling_hash.hpp"
#include "bottomk.hpp"

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    int count = 0;
    int q = 0;
    int64_t kept = 0;
    const double MIN_AUTHORS = 5;
    Merger merger({argv[1], argv[2]});
    SlidingWindow sw(100000);
    std::unordered_map<int64_t, std::pair<std::string, std::string>> reverse_map; //create struct
    std::unordered_set<int64_t> known_hitters;
    std::unordered_map<int64_t, BottomK> bottomk_map;

    while(true){
    try {
        std::cout.flush();
        std::optional<Record> record = merger.merged_next();
        if(!record.has_value()) break;
        std::vector<std::string> tokens = tokenize(record->text);
        rolling_hash(sw, tokens, record->timestamp, record->link, record->author, reverse_map, known_hitters, bottomk_map);
        count++;
        if(count % 10000 == 0) std::cout << "processed: " << count << std::endl;
        if(count % 100000 == 0) { 
            ++q; 
            auto hitters = sw.query(); 

            for(auto& [hash, freq] : hitters) {
                auto it = bottomk_map.find(hash);
                if(it == bottomk_map.end()) continue;
                double authors = it->second.estimate();
                if(authors < MIN_AUTHORS) continue;
                ++kept;
                if(reverse_map.count(hash)) {
                    std::cout << "phrase: " << reverse_map[hash].first
                            << " | count: " << freq
                            << " | authors~" << authors
                            << " | link: " << reverse_map[hash].second
                            << std::endl;
                }
            }
            std::cout << "query " << q << " — heavy hitters: " << hitters.size() << ", after author filtering: " << kept << std::endl;

            for(auto& [hash, freq] : hitters) {
                known_hitters.insert(hash);
            }

            std::unordered_map<int64_t, std::pair<std::string, std::string>> pruned;
            for(auto& hash : known_hitters) {
                if(reverse_map.count(hash)) {
                    pruned[hash] = reverse_map[hash];
                }
            }
            reverse_map = std::move(pruned);
        }
    } catch(const std::exception& e) {
        std::cout << "exception at record " << count << ": " << e.what() << std::endl;
        break;
    }
}

    auto hitters = sw.query();
    std::cout << "final query — heavy hitters: " << hitters.size() << ", after author filtering: " << kept << std::endl;

    return 0;

}