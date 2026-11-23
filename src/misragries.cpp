#include "misragries.hpp"

#include <algorithm>
#include <vector>

// insert new element in the sketch
void MisraGries::insert(int64_t x){
    ++m; // increment elements_seen counter

    // search for hash in table: increment if it exists, create it otherwise
    if(auto search = H.find(x); search != H.end()) { 
        search->second++; 
    } 
    else { 
        H[x] = 1; 
    }

    // if table is full: decrement all counters, erase the ones that reach 0
    if(H.size() >= s){
        for(auto iter = H.begin(); iter != H.end();){
            iter->second--;
            if(iter->second == 0) { iter = H.erase(iter); }
            else { ++iter; }
        }
    }
}

// comparator helper for merging
bool comp(std::pair<int64_t, int64_t> a, std::pair<int64_t, int64_t> b){
    return a.second > b.second;
}

// merge two existing sketches
void MisraGries::merge(const MisraGries& other){
    // merge this->H and other.H into merged
    std::unordered_map<int64_t, int64_t> merged = this->H;
    for(auto [key, val] : other.H){
        merged[key] += val;
    }

    // sort the merged hash in decreasing order
    std::vector<std::pair<int64_t, int64_t>> ordered(merged.begin(), merged.end());

    // if ordered is full: 
    if(ordered.size() >= s){
        std::nth_element(ordered.begin(), ordered.begin() + (s - 1), ordered.end(), comp);
        int64_t sth_biggest = ordered[s - 1].second;                            // get the sth biggest counter (sorted)

        for(auto iter = merged.begin(); iter != merged.end();){                 // while iterating on merged
            iter->second -= sth_biggest;                                        // decrement all counters
            if(iter->second <= 0) { 
                iter = merged.erase(iter);                                      // erase the ones that reach 0
            }
            else { 
                ++iter;
            }
        }
    }

    // update H (merged as modified) and m (sum of all ements seen by this->H and other.H)
    this->H = merged;
    this->m += other.m;
}

// get counters above the sketch's high frequency threshold
std::unordered_map<int64_t, int64_t> MisraGries::get_frequencies(){
    std::unordered_map<int64_t, int64_t> high_freq;
    int64_t threshold = this->m / this->s;

    for(const auto& [key, val]: this->H){
        if(val > threshold) { 
            high_freq[key] = val;                                               // copy (hash, val) if val > m/s
        }
    }
    return high_freq;
}