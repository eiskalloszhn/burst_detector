#include "slidingwindow.hpp"
#include "bottomk.hpp"

#include <unordered_set>
#include <random>

const static int k = 200;
const static int64_t q = 2305843009213693951LL;
const static int64_t p = [](){
    std::mt19937_64 gen(12345);
    return (int64_t)(gen() % (q - 2000000000LL)) + 1000000000LL;
}();
const static int64_t p1 = (__int128)p * p%q;
const static int64_t p2 = (__int128)p1 * p%q;
const static int64_t p3 = (__int128)p2 * p%q;
const static int64_t p4 = (__int128)p3 * p%q;

void rolling_hash(SlidingWindow& sw, 
                const std::vector<std::string>& tokens, 
                int64_t timestamp, 
                const std::string& link, 
                const std::string& author,
                std::unordered_map<int64_t, std::pair<std::string, std::string>>& reverse_map,
                std::unordered_set<int64_t>& known_hitters,
                std::unordered_map<int64_t, BottomK>& bottomk_map){

    if(tokens.size() < 5) return;

    auto token_to_int = [&](const std::string& token) -> int64_t {
        int64_t result = 0;
        for(unsigned char c: token){
            result = ((__int128) result * p + c) % q;
        }
        return result;
    };

    auto tokens_to_phrase = [&](int start) -> std::string {
    return tokens[start] + " " + tokens[start+1] + " " + 
           tokens[start+2] + " " + tokens[start+3] + " " + tokens[start+4];
    };

    int64_t first_hash = (int64_t)( (__int128)token_to_int(tokens[0])*p4 %q + 
                                    (__int128)token_to_int(tokens[1])*p3 %q + 
                                    (__int128)token_to_int(tokens[2])*p2 %q + 
                                    (__int128)token_to_int(tokens[3])*p1 %q + 
                                    (__int128)token_to_int(tokens[4]) )
                                    % q;
    
    if(known_hitters.count(first_hash)){
        auto author_hash = ( (double)token_to_int(author) + 1 ) / (double)q;
        
        if(bottomk_map.find(first_hash) == bottomk_map.end()){
            bottomk_map.emplace(first_hash, BottomK(k));
        }

        bottomk_map[first_hash].insert_sketch(author_hash);
    }

    if(reverse_map.find(first_hash) == reverse_map.end()){
            reverse_map[first_hash] = {tokens_to_phrase(0), link};
    }

    sw.process(first_hash, timestamp);

    int64_t roll_hash = first_hash;
        for(int pos = 5; pos <= tokens.size() - 1; pos++){
            roll_hash = (int64_t)(((__int128)(roll_hash - ((__int128)token_to_int(tokens[pos - 5])*p4 % q) + q) * p ) + token_to_int(tokens[pos]) ) %q;
            
            if(known_hitters.count(roll_hash)){
                auto author_hash = (double)token_to_int(author) / (double)q;
                
                if(bottomk_map.find(roll_hash) == bottomk_map.end()){
                    bottomk_map.emplace(roll_hash, BottomK(k));
                }

                bottomk_map[roll_hash].insert_sketch(author_hash);
            }

            if(reverse_map.find(roll_hash) == reverse_map.end()){
                reverse_map[roll_hash] = {tokens_to_phrase(pos - 4), link};
            }

            sw.process(roll_hash, timestamp);
        }
}