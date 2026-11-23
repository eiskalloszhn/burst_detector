#include "bottomk.hpp"

#include <set>

// insert value into sketch
void BottomK::insert_sketch(double value){
    ++n;                                                        // increment elements seen

    if(bottomk.count(value)) return;                            // ignore if duplicate

    if(bottomk.size() < k) {                                    // insert if new value && not full
        bottomk.insert(value);
    }
    else if(*bottomk.rbegin() > value ) {                       // if current value is smaller than the kth smallest stored
        bottomk.erase(*bottomk.rbegin());                       // erase the kth smallest
        bottomk.insert(value);                                  // insert the current one
    }
} 

// estimate distint elements seen in the stream
double BottomK::estimate(){
    if(bottomk.size() < k) return (double) bottomk.size();      // return exact count when elements seen < k
    return (double) (k - 1) / *bottomk.rbegin();                // unbiased etimator of F0
}