#include <vector>
#include <sstream>
#include <string>
#include <algorithm>

// Tokenization funciton
std::vector<std::string> tokenize(const std::string& text){
    std::istringstream is(text);
    
    std::string token;
    std::vector<std::string> output;

    while(is >> token){
        std::transform(token.begin(), token.end(), token.begin(), tolower); // lowercase all tokens
        if(token.length() > 1 || token.rfind("http", 0)) {                  // discard tokens shorter tha 1 char and links
            output.push_back(token); 
        }
    }
    return output;
}