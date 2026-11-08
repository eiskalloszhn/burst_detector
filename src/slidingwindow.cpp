#include "slidingwindow.hpp"
#include <cmath>

static const int B = 3;

bool SlidingWindow::merge(){
    int start = 0;
    for(int i = 1; i < (int)buckets.size(); i++){
        if(buckets[i].level != buckets[i-1].level) {
            start = i;
        } else if(i - start + 1 == B + 2){
            buckets[start+1].mg.merge(buckets[start].mg);
            buckets[start+1].element_count += buckets[start].element_count;
            buckets[start+1].oldest_timestamp = buckets[start].oldest_timestamp;
            buckets[start+1].level += 1; 
            buckets.erase(buckets.begin() + start);
            return true;
        }
    }
    return false;
}

void SlidingWindow::cascade_merge(){
    while(merge()) {}
}

void SlidingWindow::process(int64_t hash, int64_t timestamp){
    current.mg.insert(hash);
    current.element_count++;
    
    if(current.oldest_timestamp == 0) current.oldest_timestamp = timestamp;
    current.newest_timestamp = timestamp;

    if(current.newest_timestamp - current.oldest_timestamp >= BUCKET_SIZE ){
        buckets.push_back(current);
        current = Bucket(s);

        cascade_merge();

    }

    while(!buckets.empty() && buckets.front().newest_timestamp < timestamp - WINDOW_SIZE) buckets.pop_front();

}

std::unordered_map<int64_t, int64_t> SlidingWindow::query(){
    MisraGries tmp = current.mg;

    for(auto& bucket: buckets){
        tmp.merge(bucket.mg);
    }

    return tmp.get_frequencies();
}

BurstSketches SlidingWindow::split_query(){
    BurstSketches result(s);

    if(buckets.empty()) { return result; }

    result.recent = buckets.back().mg;

    for(size_t i = 0; i < buckets.size() - 1; i++){
        result.baseline.merge(buckets[i].mg);
    }
    return result;
}

std::unordered_map<int64_t, double> SlidingWindow::burst_scores(){
    BurstSketches bsk = split_query();
    std::unordered_map<int64_t, double> scores;

    double recent_m = (double) bsk.recent.get_m();
    double base_m = (double) bsk.baseline.get_m();

    if(recent_m == 0 || base_m < s) { return scores; }

    for(auto& [hash, count]: bsk.recent.get_frequencies()){
        double recent_freq = (double) count / recent_m;
        double base_count = (double) bsk.baseline.count_of_hash(hash) + base_m / s;
        double base_freq = (double) base_count / base_m;

        scores[hash] = recent_freq / base_freq;
    }
    return scores;
}