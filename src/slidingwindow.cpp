#include "slidingwindow.hpp"

#include <cmath>

// merge consecutive Buckets of the same level (at most B+1 Buckets can exist)
bool SlidingWindow::merge(){
    int start = 0;
    for(int i = 1; i < (int)buckets.size(); i++){
        if(buckets[i].level != buckets[i-1].level) {                                // if level changes
            start = i;                                                              // reset start index
        } else if(i - start + 1 == B + 2){                                          // if there are B+2 Buckets of the same level                                  
            buckets[start+1].mg.merge(buckets[start].mg);                           // merge Buckets and update all parameters
            buckets[start+1].element_count += buckets[start].element_count;
            buckets[start+1].oldest_timestamp = buckets[start].oldest_timestamp;
            buckets[start+1].level += 1; 
            buckets.erase(buckets.begin() + start);
            return true;                                                            // trigger cascade loop
        }
    }
    return false;                                                                   // interrupt cascade loop
}

// cascade merge calls to check over all the levels to enforce DGIM Bucket bound
void SlidingWindow::cascade_merge(){
    while(merge()) {}
}

// process new stream element, seal Buckets and expire old ones
void SlidingWindow::process(int64_t hash, int64_t timestamp){
    current.mg.insert(hash);                                                        // update current sketch and count
    current.element_count++;
    
    if(current.oldest_timestamp == 0) {                                             // if first element in Bucket, save oldest_timestamp
        current.oldest_timestamp = timestamp;
    }
    current.newest_timestamp = timestamp;                                           // update newest_timestamp
    
    // seal Bucket when BUCKET_SIZE is reached, push into queue and merge
    if(current.newest_timestamp - current.oldest_timestamp >= BUCKET_SIZE ){
        ++sealed;                                                                   // increase sealed counter
        buckets.push_back(current);                                                 // push sealed Bucket into queue
        current = Bucket(s);                                                        // create new Bucket

        cascade_merge();                                                            // enforce DGIM

    }

    // pop from queue Buckets that fall outside WINDOW_SIZE
    while(!buckets.empty() && buckets.front().newest_timestamp < timestamp - WINDOW_SIZE) {
        buckets.pop_front();
    }

}

// query() is not used anymore: I keep it to show the case where 
// detection is done without baseline comparisons:
// it is substituted by split_query() together with burst_scores()

// merge all Buckets in queue, return heavy hitters
std::unordered_map<int64_t, int64_t> SlidingWindow::query(){
    MisraGries tmp = current.mg;

    for(auto& bucket: buckets){
        tmp.merge(bucket.mg);
    }

    return tmp.get_frequencies();
}

// split Buckets queue into current activity and baseline
BurstSketches SlidingWindow::split_query(){
    BurstSketches result(s);

    if(buckets.empty()) { 
        return result; 
    }

    result.recent = buckets.back().mg;                                              // current acitivy id the most recent Bucket

    for(size_t i = 0; i < buckets.size() - 1; i++){
        result.baseline.merge(buckets[i].mg);                                       // merge all the other Buckets for baseline
    }
    
    return result;
}

// compute burst scores using split_query()
std::unordered_map<int64_t, double> SlidingWindow::burst_scores(){
    BurstSketches bsk = split_query();
    std::unordered_map<int64_t, double> scores;

    double recent_m = (double) bsk.recent.get_m();                                  // total elements in recent
    double base_m = (double) bsk.baseline.get_m();                                  // total elements in baselin

    if(recent_m == 0 || base_m < s) {                                               // skip if insufficient data
        return scores; 
    }

    for(auto& [hash, count]: bsk.recent.get_frequencies()){                         // for the heavy hitters in recent
        double recent_freq = (double) count / recent_m;                             // compute its frequency over all elements in recent
        double base_count = (double) bsk.baseline.count_of_hash(hash) + base_m / s; // estimated frequency + Misra Gries error to peevent false positives and /0
        double base_freq = (double) base_count / base_m;                            // compute the heavy hitter's frequency in the baseline

        scores[hash] = recent_freq / base_freq;                                     // compute brust score as the ratio between recent activity and baseline frequency
    }
    return scores;
}