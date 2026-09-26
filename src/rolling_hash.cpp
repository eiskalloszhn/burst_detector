#include "slidingwindow.hpp"

const static int64_t p = 31;
const static int64_t q = 2305843009213693951LL;
const static int64_t p1 = (__int128)p * p%q;
const static int64_t p2 = (__int128)p1 * p%q;
const static int64_t p3 = (__int128)p2 * p%q;
const static int64_t p4 = (__int128)p3 * p%q;

void rolling_hash(SlidingWindow& sw, const std::vector<std::string>& tokens, int64_t timestamp){
    if(tokens.size() < 5) return;

    auto token_to_int = [&](const std::string token) -> int64_t {
        int64_t result = 0;
        for(char c: token){
            result = ((__int128) result * p + c) % q;
        }
        return result;
    };

    int64_t first_hash = (int64_t)( (__int128)token_to_int(tokens[0])*p4 %q + 
                                    (__int128)token_to_int(tokens[1])*p3 %q + 
                                    (__int128)token_to_int(tokens[2])*p2 %q + 
                                    (__int128)token_to_int(tokens[3])*p1 %q + 
                                    (__int128)token_to_int(tokens[4]) )
                                    % q;
    sw.process(first_hash, timestamp);

    int64_t roll_hash = first_hash;
        for(int pos = 5; pos <= tokens.size() - 1; pos++){
            roll_hash = (int64_t)(((roll_hash - ((__int128)token_to_int(tokens[pos - 5])*p4 % q) + q) * p ) + token_to_int(tokens[pos]) ) %q;
            sw.process(roll_hash, timestamp);
        }
}