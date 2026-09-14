#pragma once
#include <optional>
#include "reader.hpp"
#include "parser.hpp"

class Merger{
    
    public:
        Merger(const std::array<std::string, 2>& filepaths);
        std::optional<Record> merged_next();
        bool has_next();
    
    private:
        std::array<ZSTDReader, 2> readers;
        std::array<std::optional<Record>, 2> candidates;
        int smallest_index();
};