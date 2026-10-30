#include "bottomk.hpp"
#include <set>

void BottomK::insert_sketch(double value){
    ++n;
    if(bottomk.count(value)) return;
    if(bottomk.size() < k) { bottomk.insert(value); }
    else if(bottomk.size() == k) {
        if(*bottomk.rbegin() > value ) {
            bottomk.erase(*bottomk.rbegin());
            bottomk.insert(value);
        }
    } // skip sketch full case if no substitution can be done
}

double BottomK::estimate(){
    if(bottomk.size() < k) return (double) bottomk.size();
    return (double) k / *bottomk.rbegin();
}