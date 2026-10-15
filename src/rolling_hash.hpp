#include "slidingwindow.hpp"
#include "bottomk.hpp"

void rolling_hash(SlidingWindow& sw, 
                const std::vector<std::string>& tokens, 
                int64_t timestamp, 
                const std::string& link, 
                const std::string& author,
                std::unordered_map<int64_t, std::pair<std::string, std::string>>& reverse_map,
                std::unordered_set<int64_t>& known_hitters,
                std::unordered_map<int64_t, BottomK>& bottomk_map);