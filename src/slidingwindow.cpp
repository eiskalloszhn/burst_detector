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