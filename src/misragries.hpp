#pragma once

#include <optional>
#include <map>
#include <vector>
#include <algorithm>

class MisraGries{

    public:
        MisraGries(int s) : s(s), m(0) {}
        void insert(int64_t x);
        void merge(const MisraGries& other);
        std::unordered_map<int64_t, int64_t> get_frequencies(); // no map= O(1) vs O(log n) access
        size_t counters() const { return H.size(); }
        int64_t count_of_hash(int64_t x) const {
            if(auto search = H.find(x); search != H.end()){ return search->second; }
            else { return 0; }
        };
        int64_t get_m() const { return m; }

    private:
        int s;      // counter
        int64_t m;  // elements seen
        std::unordered_map<int64_t, int64_t> H;
};