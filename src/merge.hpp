#pragma once

#include <optional>
#include <array>
#include <string>

#include "reader.hpp"
#include "parser.hpp"

class Merger{
    
    public:
        Merger(const std::array<std::string, 2>& filepaths);
        std::optional<Record> merged_next();                    // fetch next value from merged Records
    
    private:
        std::array<ZSTDReader, 2> readers;                      // ZSTD Reader per file
        std::array<std::optional<Record>, 2> candidates;        // candidates for next in merging order
        int smallest_index();                                   // track smallest timestamp for sorting
};