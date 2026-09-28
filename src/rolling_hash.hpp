#include "slidingwindow.hpp"

void rolling_hash(SlidingWindow& sw, const std::vector<std::string>& tokens, int64_t timestamp, const std::string& link, std::unordered_map<int64_t, std::pair<std::string, std::string>>& reverse_map);