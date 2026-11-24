#pragma once

#include "slidingwindow.hpp"
#include "bottomk.hpp"

#include <unordered_set>
#include <string>
#include <vector>
#include <unordered_map>

using Phrase     = std::pair<std::string, std::string>;                 // (5-gram text, link of first occurrence)
using ReverseMap = std::unordered_map<int64_t, Phrase>;                 // (hash fingerprint, element data)

// Compute Karp-Rabin polynomial hashing over 5-grams
// updating ReverseMap, Sliding Window, and BottomK sketch
void rolling_hash(SlidingWindow& sw, 
                const std::vector<std::string>& tokens, 
                int64_t timestamp, 
                const std::string& link, 
                const std::string& author,
                ReverseMap& reverse_map,
                std::unordered_set<int64_t>& known_hitters,
                std::unordered_map<int64_t, BottomK>& bottomk_map);