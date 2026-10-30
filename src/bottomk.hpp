#pragma once
#include <set>

class BottomK{
    public:
        BottomK() : k(400) {}
        BottomK(int k) : k(k) {}
        void insert_sketch(double value);
        double estimate();
        int64_t occurrences() const { return n; }
    
    private:
        int k;
        std::set<double> bottomk;
        int64_t n = 0;

};