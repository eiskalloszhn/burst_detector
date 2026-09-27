#include "misragries.hpp"
#include <iostream>

void MisraGries::insert(int64_t x){
    ++m;

    if(auto search = H.find(x); search != H.end()) { search->second++; } 
    else { H[x] = 1; }

    if(H.size() >= s){
        for(auto iter = H.begin(); iter != H.end();){
            iter->second--;
            if(iter->second == 0) { iter = H.erase(iter); }
            else { ++iter; }
        }
    }
}

bool comp(std::pair<int64_t, int64_t> a, std::pair<int64_t, int64_t> b){
    return a.second > b.second;
}

void MisraGries::merge(const MisraGries& other){
    std::unordered_map<int64_t, int64_t> merged = this->H;
    for(auto [key, val] : other.H){
        merged[key] += val;
    }

    std::vector<std::pair<int64_t, int64_t>> ordered(merged.begin(), merged.end());
    std::sort(ordered.begin(), ordered.end(), comp);

    if(ordered.size() > s){
        int64_t sth_biggest = ordered[s-1].second;

        for(auto iter = merged.begin(); iter != merged.end();){
            iter->second -= sth_biggest;
            if(iter->second <= 0) { iter = merged.erase(iter); }
            else { ++iter; }
        }
    }

    this->H = merged;
    this->m += other.m;
}

std::unordered_map<int64_t, int64_t> MisraGries::get_frequencies(){
    std::cout << "get_frequencies: H.size()=" << H.size() << " m=" << m << " threshold=" << m/s << std::endl;
    std::unordered_map<int64_t, int64_t> high_freq;
    for(auto iter = this->H.begin(); iter != this->H.end();){
        if(iter->second > this->m / this->s ) { high_freq[iter->first] = iter->second; ++iter; }
        else { ++iter; }
    }
    return high_freq;
}