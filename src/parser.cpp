#include "json.hpp"
#include "record.hpp"

#include <cctype>
#include <algorithm>
#include <regex>
#include <string>
#include <iostream>

using json = nlohmann::json;

// accounts for incongruence in file data: handles created_utc field as string and int
int64_t extract_timestamp(const json& obj){
    if(obj.at("created_utc").is_string()){
        return std::stoll(obj.at("created_utc").get<std::string>());
    } else { return obj.at("created_utc").get<int64_t>(); }
}

// Strip unwanted characters (special chars, emojis, punctuation)

// check structure of byte by counting leading 1s
int leading(unsigned char byte){
    int count = 0;
    unsigned char mask = 0b10000000;
    do {
        unsigned char masked = mask & byte;
        if(masked == 0b10000000) { count++; byte = byte << 1; } else { break; }
    } while ( count < 8 );
    return count;
}

// decode chars to check point range
int utf8_decode(std::string& text, size_t pos, int& out_len){
    // get first byte, check leading 
    unsigned char first_byte = text[pos];
    int byte_count = leading(first_byte);
    unsigned long long decoded;
    int shift = (byte_count-1) *6;
        
    if(byte_count == 0) { out_len = 1; first_byte = first_byte & 0b01111111; return (int) first_byte; }
        else if(byte_count == 2) { out_len = 2; first_byte = first_byte & 0b00011111; }
        else if(byte_count == 3) { out_len = 3; first_byte = first_byte & 0b00001111; }
        else if(byte_count == 4) { out_len = 4; first_byte = first_byte & 0b00000111; }
        else { out_len = 1; first_byte = first_byte & 0b00000000; return (int) first_byte; }

    // update with first byte and corresponding shift
    decoded = first_byte << shift;
    
    // decrease count, decode continuation bytes until count is 0
    byte_count -= 1;
    do{
        shift = (byte_count-1) *6; // get new shift
        // get byte in next pos, catch out of range
        pos += 1;
        unsigned char next_pos_value;
        try{ 
            next_pos_value = text.at(pos); 
        } 
        catch(std::out_of_range const& error) { 
            std::cout << error.what() << std::endl;
            out_len = 1;
            return 0;
        }
        // decode, shift, insert in decoded using OR, decrease count
        unsigned char next_byte = next_pos_value & 0b00111111;
        unsigned long long x = next_byte << shift; 
        decoded = decoded | x;
        byte_count -= 1;
    } while(byte_count != 0);

    return (int) decoded;
}

// deletes byte if in code point ranges
std::string stripper(std::string& text){
    std::string output;
    size_t pos = 0;
    while(pos < text.length()){
        int len;
        int code_point = utf8_decode(text, pos, len);
        bool is_emoji = (code_point >= 0x2600 && code_point <= 0x27BF) ||
                        (code_point >= 0x2B00 && code_point <= 0x2BFF) ||
                        (code_point >= 0x1F300 && code_point <= 0x1F5FF) ||
                        (code_point >= 0x1F600 && code_point <= 0x1F64F) ||
                        (code_point >= 0x1F680 && code_point <= 0x1F6FF) ||
                        (code_point >= 0x1F900 && code_point <= 0x1FAFF);
        bool is_punctuation =   (code_point >= 0x21 && code_point <= 0x2F && code_point != 0x27) ||
                                (code_point >= 0x3A && code_point <= 0x40) ||
                                (code_point >= 0x5B && code_point <= 0x60) ||
                                (code_point >= 0x7B && code_point <= 0x7E);

        if(!is_emoji && !is_punctuation){
            output.append(text, pos, len);
        }
        pos += len;
    }
    return output;
}

// Parser for json fields

std::optional<Record> parse_record(std::string& line){
    json rec;
    Record result;

    try{
        rec = json::parse(line);
        bool is_sub = rec.contains("title"); // consistent discriminating field

        result.typeflag = is_sub;
        result.author = rec["author"].get<std::string>();
        result.timestamp = extract_timestamp(rec);
        
        // different fields in subs and comms
        if(is_sub) {
            
            result.link = "https://www.reddit.com" + rec["permalink"].get<std::string>();
            
            result.text = rec["title"].get<std::string>() + " " + // load title into text
                // load text, default empty string if text is empty or not string
                (rec.contains("selftext") && rec["selftext"].is_string() ? rec["selftext"].get<std::string>() : std::string());
        
        } else { // is_comment
            
            if(rec.contains("permalink")) {
                result.link = "https://www.reddit.com" + rec["permalink"].get<std::string>();
            
            } else { // build permalink for older comments where it was not stored
            int len = rec["link_id"].get<std::string>().length();
            result.link = "https://www.reddit.com/r/memes/comments/"
                + rec["link_id"].get<std::string>().substr(3, len-3) 
                + "/_/" + rec["id"].get<std::string>();
            }
            
            result.text = rec["body"].get<std::string>(); // comments can only be strings and cannot be empty
        }  
    } 
    catch(std::exception& e){ return std::nullopt; }

    result.text = stripper(result.text);

    // check whether final text field is empty or only whitespaces
    std::string trimmed = result.text;
    trimmed.erase(remove_if(trimmed.begin(), trimmed.end(), [](unsigned char c){ return std::isspace(c); }), trimmed.end());
    if(trimmed.empty()) { return std::nullopt; }

    return result;
}