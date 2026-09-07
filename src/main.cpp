#include <iostream>
#include <optional>
#include "json.hpp"
#include "reader.hpp"
#include "parser.hpp"
#include "merge.hpp"
using json = nlohmann::json;

auto time_it = [](auto&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
};


int main() {

    Merger merger({"/Users/nic/Downloads/reddit/subreddits25/memes_submissions.zst", 
                   "/Users/nic/Downloads/reddit/subreddits25/memes_comments.zst"});

    int i = 0;
    int mult = 10;
    auto ms = time_it([&]() {
    while(merger.has_next()){
        i++;
        std::optional<Record> record = merger.merged_next();
        //std::cout << record->author << " " << record->text << " " << record->typeflag << '\n' << std::endl;
        if(i==mult) { std::cout<< i << '\n' << std::endl; mult*=10;}

        if(i==2000) { break; }
    }});

    std::cout << "Time: " << ms << " ms" << std::endl;
    return 0;
}