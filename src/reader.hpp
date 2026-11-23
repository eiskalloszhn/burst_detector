#pragma once

#include <fstream>
#include <zstd.h>
#include <vector>
#include <optional>
#include <string>

class ZSTDReader{
    
    public:
        ZSTDReader(const std::string& filepath);
        ~ZSTDReader();
        std::optional<std::string> next_line();
    
    private:
        std::ifstream file;
        ZSTD_DStream* dstream;
        std::vector<char> in_vec;
        ZSTD_inBuffer input;
        std::vector<char> out_vec;
        ZSTD_outBuffer output;
        std::string tailing;
        size_t head = 0;

};