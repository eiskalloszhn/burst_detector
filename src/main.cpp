#include <iostream>
#include <optional>
#include <chrono>
#include "json.hpp"
#include "reader.hpp"
#include "parser.hpp"
#include "merge.hpp"
#include "tokenizer.hpp"
#include "rolling_hash.hpp"
using json = nlohmann::json;

int main(int argc, char* argv[]) {
    int count = 0;
    int q = 0;
    Merger merger({argv[1], argv[2]});
    SlidingWindow sw(20000);
    while(true){
    try {
        std::cout.flush();
        std::optional<Record> record = merger.merged_next();
        if(!record.has_value()) break;
        std::vector<std::string> tokens = tokenize(record->text);
        rolling_hash(sw, tokens, record->timestamp);
        count++;
        if(count % 10000 == 0) std::cout << "processed: " << count << std::endl;
        if(count % 100000 == 0) { ++q; auto hitters = sw.query(); std::cout << "query " << q << " — heavy hitters: " << hitters.size() << std::endl;}
    } catch(const std::exception& e) {
        std::cout << "exception at record " << count << ": " << e.what() << std::endl;
        break;
    }
}

    auto hitters = sw.query();
    std::cout << "final query — heavy hitters: " << hitters.size() << std::endl;

    return 0;

}