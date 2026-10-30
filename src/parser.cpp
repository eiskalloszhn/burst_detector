#include "json.hpp"
#include "record.hpp"
#include <regex>
#include <string>
#include <iostream>

using json = nlohmann::json;

static std::regex url("https?://\\S+");
static std::regex removed("\\[removed\\]", std::regex_constants::icase);
static std::regex deleted("\\[deleted\\]", std::regex_constants::icase);

int64_t extract_timestamp(const json& obj){
    if(obj["created_utc"].is_string()){
        return std::stoll(obj["created_utc"].get<std::string>());
    } else { return obj["created_utc"].get<int64_t>(); }
}

/// unwanted char stripper
/// leading checks the structure: 0: 1 byte, 1: cont byte, 2: 2 byte...
int leading(unsigned char byte){
    int count = 0;
    unsigned char mask = 0b10000000;
    do {
        unsigned char masked = mask & byte;
        if(masked == 0b10000000) { count++; byte = byte << 1; } else { break; }
    } while ( count < 8 );
    return count;
}

/// decode chars to check point range
int utf8_decode(std::string& text, size_t pos, int& out_len){
    unsigned char first_byte = text[pos];
    int byte_count = leading(first_byte);
    unsigned long long decoded;
    int shift = (byte_count-1) *6;
        
    if(byte_count == 0) { out_len = 1; first_byte = first_byte & 0b01111111; return (int) first_byte; }
        else if(byte_count == 2) { out_len = 2; first_byte = first_byte & 0b00011111; }
        else if(byte_count == 3) { out_len = 3; first_byte = first_byte & 0b00001111; }
        else if(byte_count == 4) { out_len = 4; first_byte = first_byte & 0b00000111; }
        else { out_len = 1; first_byte = first_byte & 0b00000000; return (int) first_byte; }

    decoded = first_byte << shift;
    byte_count -= 1;

    do{
        shift = (byte_count-1) *6;
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
        unsigned char next_byte = next_pos_value & 0b00111111;
        unsigned long long x = next_byte << shift; 
        decoded = decoded | x;
        byte_count -= 1;
    } while(byte_count != 0);

    return (int) decoded;
}

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
            output += text.substr(pos, len);
        }

        pos += len;
    }

    return output;
}



std::optional<Record> parse_record(std::string& line){
    json rec;
    Record result;

    try{
        rec = json::parse(line);
        bool is_sub = rec.contains("title");

        result.typeflag = is_sub;
        result.author = rec["author"].get<std::string>();
        result.timestamp = extract_timestamp(rec);
        
        if(is_sub) {
            result.link = "https://www.reddit.com" + rec["permalink"].get<std::string>();
            result.text = rec["title"].get<std::string>() + " " + rec["selftext"].get<std::string>();
        
        } else { // is_comment
            if(rec.contains("permalink")) {
                result.link = "https://www.reddit.com" + rec["permalink"].get<std::string>();
            } else {
            int len = rec["link_id"].get<std::string>().length();
            result.link = "https://www.reddit.com/r/memes/comments/"
                + rec["link_id"].get<std::string>().substr(3, len-3) 
                + "/_/" + rec["id"].get<std::string>();
            }
            result.text = rec["body"].get<std::string>();
        }
        
    } 
    catch(json::exception& e){ return std::nullopt; }

    result.text= std::regex_replace(result.text, url, " ");
    result.text= std::regex_replace(result.text, removed, " ");
    result.text= std::regex_replace(result.text, deleted, " ");
    result.author= std::regex_replace(result.author, removed, "AUTHOR_REMOVED");
    result.author= std::regex_replace(result.author, deleted, "AUTHOR_REMOVED");

    std::string trimmed = stripper(result.text);
    trimmed.erase(remove_if(trimmed.begin(), trimmed.end(), isspace), trimmed.end());

    if(trimmed.empty()) { return std::nullopt; }

    return result;
}