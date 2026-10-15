#pragma once
#include <set>

class BottomK{
    public:
        BottomK() : k(200) {}
        BottomK(int k) : k(k) {}
        void insert_sketch(double value);
        double estimate();
    
    private:
        int k;
        std::set<double> bottomk;

};