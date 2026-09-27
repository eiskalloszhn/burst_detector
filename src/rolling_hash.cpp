#include "slidingwindow.hpp"

const static int64_t p = 31;
const static int64_t q = 2305843009213693951LL;
const static int64_t p1 = (__int128)p * p%q;
const static int64_t p2 = (__int128)p1 * p%q;
const static int64_t p3 = (__int128)p2 * p%q;
const static int64_t p4 = (__int128)p3 * p%q;

void rolling_hash(SlidingWindow& sw, const std::vector<std::string>& tokens, int64_t timestamp, 
                const std::string& link, std::unordered_map<int64_t, std::pair<std::string, std::string>>& reverse_map){
    if(tokens.size() < 5) return;

    auto token_to_int = [&](const std::string token) -> int64_t {
        int64_t result = 0;
        for(char c: token){
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

    if(reverse_map.find(first_hash) == reverse_map.end()){
        reverse_map[first_hash] = {tokens_to_phrase(0), link};
    }
    sw.process(first_hash, timestamp);

    int64_t roll_hash = first_hash;
        for(int pos = 5; pos <= tokens.size() - 1; pos++){
            roll_hash = (int64_t)(((__int128)(roll_hash - ((__int128)token_to_int(tokens[pos - 5])*p4 % q) + q) * p ) + token_to_int(tokens[pos]) ) %q;
            
            if(reverse_map.find(roll_hash) == reverse_map.end()){
                reverse_map[roll_hash] = {tokens_to_phrase(pos - 4), link};
            }

            sw.process(roll_hash, timestamp);
        }
}