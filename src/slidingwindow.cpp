#include "slidingwindow.hpp"
#include <cmath>

int level(int64_t count){
    if(count == 0) return -1;
    return (int) std::floor(std::log2((double) count));
}

bool SlidingWindow::merge(){
    for(int i = buckets.size() - 2; i >= 0; i--){
        if(level(buckets[i].element_count) == level(buckets[i+1].element_count)){
            buckets[i+1].mg.merge(buckets[i].mg);
            buckets[i+1].element_count += buckets[i].element_count;
            buckets[i+1].oldest_timestamp = buckets[i].oldest_timestamp;
            buckets.erase(buckets.begin() + i);
            return true;
        }
    }   return false;
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