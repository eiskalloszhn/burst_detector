#include "rolling_hash.hpp"
#include "slidingwindow.hpp"
#include "bottomk.hpp"

#include <unordered_set>
#include <random>
#include <vector>
#include <cstdint>
#include <array>
#include <utility>

namespace {
    // Marsenne prime 2^61 - 1 for hashing module
    constexpr int64_t q = 2305843009213693951LL;
    
    // Two seeded, independent random base p in range [10^9, q - 10^9]
    const std::pair<int64_t, int64_t> bases = [](){
        std::mt19937_64 gen(12345);
        auto draw = [&]() -> int64_t {
            return (int64_t)( gen() % (q - 2000000000LL) ) + 1000000000LL;
        };
        int64_t a = draw();
        int64_t b = draw();
        
        while(a == b){
            b = draw();
        }

        return std::make_pair(a, b);
    }();
    const int64_t p_word = bases.first;
    const int64_t p_gram = bases.second;

    // Polynomial hash of single token
    auto token_to_int(const std::string& token){
        int64_t result = 0;
        for(unsigned char c: token){
            result = (int64_t) ( (__int128) result * p_word + c ) % q;
        }
        return result;
    };

    // Precomputed powers p_gram mod q for rolling hash:
    // Hash[k] = p_gram^k mod q for k=[0..4]
    const std::array<int64_t, 5> Hash = [](){
        std::array<int64_t, 5> h;
        h[0] = 1;
        for (int k = 1; k < 5; k++){
            h[k] = (int64_t) ( (__int128_t) h[k-1] * p_gram % q );
        }
        return h;
    }();

    // SplitMix64 for uniform bit distribution for author hashes (for BottomK correctness)
    uint64_t splitmix64(uint64_t x){
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }

    // Author uniform hash in (0, 1] with 53-bit precision
    double uniform_author_hashing(const std::string& author){
        uint64_t mixed = splitmix64( (uint64_t) token_to_int(author) );
        return ((mixed >> 11) + 1.0) / 9007199254740992.0;
    }

}

void rolling_hash(SlidingWindow& sw, 
                const std::vector<std::string>& tokens, 
                int64_t timestamp, 
                const std::string& link, 
                const std::string& author,
                ReverseMap& reverse_map,
                std::unordered_set<int64_t>& known_hitters,
                std::unordered_map<int64_t, BottomK>& bottomk_map){

    if(tokens.size() < 5) return;                                                   // at least 5 tokens per phrase

    // reconstruct 5-grams from tokens starting at given (rolling) idx
    auto tokens_to_phrase = [&](int idx) -> std::string {
    return tokens[idx] + " " + tokens[idx+1] + " " + 
           tokens[idx+2] + " " + tokens[idx+3] + " " + tokens[idx+4];
    };

    const double author_hash = uniform_author_hashing(author);

    // processing loop for 5-grams
    auto process_5gram = [&](int64_t h, size_t idx){
        if(known_hitters.count(h)){                                                 // if the hash is a known high frequency 5-gram
            bottomk_map[h].insert_sketch(author_hash);                              // update BottomK with new author found
        }

        if(reverse_map.find(h) == reverse_map.end()){                               // if it is not saved in the reverse_msp
            reverse_map.emplace(h, std::make_pair(tokens_to_phrase(idx), link));    // insert text and first occurence link with its hash as key
        }

        sw.process(h, timestamp);                                                   // process stream element in Sliding Window
    };

    // hash each token individually
    std::vector<int64_t> tokens_hash(tokens.size());
    for(size_t i = 0; i < tokens.size(); i++){
        tokens_hash[i] = token_to_int(tokens[i]);
    }

    // compute initial 5-gram hash
    int64_t roll_hash = (int64_t)( ( (__int128)tokens_hash[0]*Hash[4] %q + 
                                    (__int128)tokens_hash[1]*Hash[3] %q + 
                                    (__int128)tokens_hash[2]*Hash[2] %q + 
                                    (__int128)tokens_hash[3]*Hash[1] %q + 
                                    (__int128)tokens_hash[4] )
                                    % q );

    // process with idx 0
    process_5gram(roll_hash, 0);

    // slide the 5-gram window across the remaining tokens in the phrase one at a time
    for(size_t pos = 5; pos <= tokens.size() - 1; pos++){
        __int128 rh = roll_hash;
        rh -= (__int128)tokens_hash[pos - 5] * Hash[4] % q;                         // remove leftmost token
        if(rh < 0) rh += q;
        roll_hash = (int64_t)( (rh * p_gram + tokens_hash[pos]) % q );              // add incoming token
        
        process_5gram(roll_hash, pos - 4);                                          // process with corresponding idx
    }
}