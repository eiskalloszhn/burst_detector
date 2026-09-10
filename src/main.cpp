#include <iostream>
#include <optional>
#include <chrono>
#include "json.hpp"
#include "reader.hpp"
#include "parser.hpp"
#include "merge.hpp"
using json = nlohmann::json;

int main(int argc, char* argv[]) {
    auto start = std::chrono::steady_clock::now();
int count = 0;
ZSTDReader reader(argv[1]);
while (true) {
    std::optional<std::string> line = reader.next_line();
    if (!line.has_value()) break;
    count++;
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed >= std::chrono::seconds(30)) break;
}
std::cout << "Lines read in 30 seconds: " << count << std::endl;
return 0;

}