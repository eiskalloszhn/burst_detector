#include <iostream>
#include <optional>
#include "json.hpp"
#include "reader.hpp"
#include "parser.hpp"
#include "merge.hpp"
using json = nlohmann::json;

int main(int argc, char* argv[]) {

    std::array<std::string, 2> filepaths = { std::string(argv[1]), std::string(argv[2]) };

    Merger merger(filepaths);

    int i = 0;
    int mult = 10;
    while(merger.has_next()){
        i++;
        std::optional<Record> record = merger.merged_next();
        //std::cout << record->author << " " << record->text << " " << record->typeflag << '\n' << std::endl;
        if(i==mult) { std::cout<< i << '\n' << std::endl; mult*=10;}

        if(i==2000) { break; }
    };

    return 0;
}