#pragma once

#include <optional>
#include <map>

class MisraGries{

    public:
        MisraGries(int s);
        void insert(int64_t x);
        void merge(const MisraGries& other);
        std::unordered_map<int64_t, int64_t> get_frequencies(); // no map= O(1) vs O(log n) access

    private:
        int s;      // counter
        int64_t m;  // elements seen
        std::unordered_map<int64_t, int64_t> H;
};