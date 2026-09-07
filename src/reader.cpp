#include "reader.hpp"
#include <zstd.h>
#include <fstream>
#include <stdexcept>

ZSTDReader::ZSTDReader(const std::string& filepath){
    file.open(filepath, std::ifstream::binary);
    dstream = ZSTD_createDStream();
    
    size_t const in_size = ZSTD_DStreamInSize();
    size_t const out_size = ZSTD_DStreamOutSize();
    in_vec.resize(in_size);
    out_vec.resize(out_size);

    input = {.src = in_vec.data(), .size=0, .pos=0};
    output = {.dst = out_vec.data(), .size=out_size, .pos=0};
}

ZSTDReader::~ZSTDReader(){
    ZSTD_freeDStream(dstream);
}

std::optional<std::string> ZSTDReader::next_line(){
    while (true){
        if (input.pos == input.size) {
            file.read(in_vec.data(), in_vec.size());
        
            if (file.gcount() == 0) { 
                if (tailing.empty()) { return std::nullopt; }
                else { std::string ret = tailing; tailing.clear(); return ret; }
        } else { input.size = file.gcount(); } 
        
            input.pos = 0;
        }

        output.pos = 0;
        size_t val = ZSTD_decompressStream(dstream, &output, &input);
        
        if (ZSTD_isError(val)) {
            throw std::runtime_error(ZSTD_getErrorName(val));
        } else {
            std::string chunk(out_vec.data(), output.pos);
            tailing += chunk;
            size_t str_len = tailing.length();

            size_t newline_pos = tailing.find('\n');
            if (newline_pos != std::string::npos) {
                std::string newline = tailing.substr(0, newline_pos);
                tailing = tailing.substr(newline_pos+1, str_len-(newline_pos+1));
                return newline;
            }
        }
    }
}