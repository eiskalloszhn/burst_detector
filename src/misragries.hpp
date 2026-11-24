#pragma once

#include <optional>
#include <map>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cstdint>

// MisraGries sketch for heavy hitter detection:
//  – insert new hash, or increase existing counter
//  – merge sketch tables when table is full
//  – get frequencies higher than the threshold

class MisraGries{

    public:
        MisraGries(size_t s) : s(s), m(0) {}
        void insert(int64_t x);                                 // insert element in sketch
        void merge(const MisraGries& other);                    // merge two sketches
        std::unordered_map<int64_t, int64_t> get_frequencies(); // unordered = O(1) access
        
        size_t counters() const { return H.size(); }            // hash table size
        int64_t count_of_hash(int64_t x) const {                // get estimated frequency of x
            if(auto search = H.find(x); search != H.end()){ return search->second; }
            else { return 0; }
        };
        int64_t get_m() const { return m; }                     // return elements seen so far

    private:
        size_t s;                                               // counter
        int64_t m;                                              // elements seen
        std::unordered_map<int64_t, int64_t> H;                 // hash table
};