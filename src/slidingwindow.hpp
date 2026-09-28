#pragma once

#include "misragries.hpp"
#include <map>
#include <deque>

static const int64_t WINDOW_SIZE = 15778476LL; // 6 months
static const int64_t BUCKET_SIZE = 2629746LL; // 1 month

struct Bucket{
    MisraGries mg;
    int64_t oldest_timestamp;
    int64_t newest_timestamp;
    int64_t element_count;

    Bucket(int s) : mg(s), oldest_timestamp(0), newest_timestamp(0), element_count(0) {}
};

class SlidingWindow{
    public:
        SlidingWindow(int s) : s(s), current(s) {}
        void process(int64_t hash, int64_t timestamp);
        std::unordered_map<int64_t, int64_t> query();


    private:
        bool merge();
        void cascade_merge();
        int s; // matching with MisraGries parameter
        std::deque<Bucket> buckets;
        Bucket current;
};