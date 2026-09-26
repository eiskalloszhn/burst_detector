#include <vector>
#include <sstream>
#include <string>
#include <algorithm>

std::vector<std::string> tokenize(const std::string& text){
    std::istringstream is(text);
    
    std::string token;
    std::vector<std::string> output;

    while(is >> token){
        std::transform(token.begin(), token.end(), token.begin(), tolower);
        if(token.length() > 1) { output.push_back(token); }
    }

    return output;
}