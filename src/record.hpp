#pragma once
#include <string>

struct Record{
    bool typeflag; // true if submission, false if comment
    std::string author;
    int64_t timestamp;
    std::string link;
    std::string text;
};