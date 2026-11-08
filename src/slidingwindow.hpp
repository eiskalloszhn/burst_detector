#pragma once

#include "misragries.hpp"
#include <map>
#include <deque>

static const int64_t WINDOW_SIZE = 7889238LL; // 3 months
static const int64_t BUCKET_SIZE = 604800LL; 

struct Bucket{
    MisraGries mg;
    int64_t oldest_timestamp;
    int64_t newest_timestamp;
    int64_t element_count;
    int level;

    Bucket(int s, int level = 0) : mg(s), oldest_timestamp(0), newest_timestamp(0), element_count(0) {}
};

struct BurstSketches{
    MisraGries baseline;
    MisraGries recent;

    BurstSketches(int s): baseline(s), recent(s) {}
};

class SlidingWindow{
    public:
        SlidingWindow(int s) : s(s), current(s) {}
        void process(int64_t hash, int64_t timestamp);
        std::unordered_map<int64_t, int64_t> query();
        size_t num_buckets() const { return buckets.size(); }
        size_t total_counters() const {
            size_t t = current.mg.counters();
            for(const auto& b : buckets) {
                t += b.mg.counters();
            } return t;
        }

        BurstSketches split_query();
        std::unordered_map<int64_t, double> burst_scores();

    private:
        bool merge();
        void cascade_merge();
        int s; // matching with MisraGries parameter
        std::deque<Bucket> buckets;
        Bucket current;
};