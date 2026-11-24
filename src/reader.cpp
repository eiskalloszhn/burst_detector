#include "reader.hpp"
#include "parser.hpp"

#include <zstd.h>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <optional>

// Open stream, decompress .zstd files and populate nput and output buffers
// as documented in the zstd.h source code

ZSTDReader::ZSTDReader(const std::string& filepath){
    file.open(filepath, std::ifstream::binary);
    if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filepath);
}
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

// next_line(): return next line or nullopt. Decompress chunk by chunk, hold in tailing
// until \n, then return

std::optional<std::string> ZSTDReader::next_line(){
    while (true){
        // if a complete line is already buffered, return it and advance head past it
        size_t newline_pos = tailing.find('\n', head); 
        if (newline_pos != std::string::npos) {
            std::string newline = tailing.substr(head, newline_pos - head);
            head = newline_pos + 1;
            return newline; 
        }
        // if the previous output was full, the decoder may still hold data, so try to decompress once
        if (input.pos == input.size && output.pos != output.size) {
            file.read(in_vec.data(), in_vec.size());
            if (file.gcount() == 0) { // if eof
                if (head == tailing.size()) { return std::nullopt; } // nothing left in tailing return nullopt
                else {                                               // last decompressed line had no /n at the end, return tailing once
                    std::string ret = tailing.substr(head); 
                    tailing.clear(); 
                    head = 0; 
                    return ret; 
                }
            } else { 
                // read chunk from file
                input.size = file.gcount(); 
            } 
            // reset position to start of chunk
            input.pos = 0;
        }
        
        // decompress
        size_t in_before = input.pos;
        output.pos = 0;
        size_t val = ZSTD_decompressStream(dstream, &output, &input);
        
        if (ZSTD_isError(val)) {
            throw std::runtime_error(ZSTD_getErrorName(val));
        } else {
            // update tailing with new chunk
            tailing.append(out_vec.data(), output.pos);
            
            if (head > 0) {                                           // if head does not point at the beginning
                tailing.erase(0, head);                               // drop the part of tailing already returned, reset head
                head = 0; 
            }
        }

        // sanity check: stream loaded correctly
        if (output.pos == 0 && input.pos == in_before) {
            if(input.pos == input.size) continue;
            throw std::runtime_error(
                "stream interrupted");
        }
    }
}