#pragma once

#include <set>
#include <cstdint>

// BottomK sketch for distinct element count (F0) estimation
class BottomK{
    public:
        static constexpr size_t K = 200;                 // sketch capacity
        
        BottomK() : k(K) {}
        void insert_sketch(double value);               // insert new value into sketch
        double estimate();                              // estimate distinct elements
        
        int64_t occurrences() const { return n; }       // return number of elements processed
    
    private:
        size_t k;                                       // max number of hashes
        std::set<double> bottomk;                       // set of k smallest hashes 
        int64_t n = 0;                                  // number of elements processed

};