#include <optional>
#include "parser.hpp"
#include "reader.hpp"
#include "merge.hpp"

std::optional<Record> get_next_valid(ZSTDReader& reader){
    while(true){
        std::optional<std::string> next = reader.next_line();
        if(!next.has_value()) { return std::nullopt; }

        std::optional<Record> record = parse_record(*next);
        if(record.has_value()) { return record; }
    }
}

int Merger::smallest_index(){
    int64_t smallest = INT64_MAX;
    int index = -1;
    
    for(int i=0; i<2; i++){
        if(candidates[0].has_value()){
            if(candidates[i]->timestamp < smallest) { smallest = candidates[i]->timestamp; index = i; }
        }
    }

    return index;
}

Merger::Merger(const std::array<std::string, 2>& filepaths) 
    : readers{ ZSTDReader(filepaths[0]), ZSTDReader(filepaths[1]) } {

    candidates = { get_next_valid(readers[0]), get_next_valid(readers[1]) };
}

std::optional<Record> Merger::merged_next(){
    
    int index = smallest_index();

    if(index == -1) { return std::nullopt; }
    
    Record result = *candidates[index];
    candidates[index] = get_next_valid(readers[index]);
    
    return result;
}

bool Merger::has_next(){
    return (smallest_index() != -1);
}

