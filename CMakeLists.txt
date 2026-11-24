cmake_minimum_required(VERSION 3.20)
project(AMD_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")

add_executable(proj
    src/main.cpp
    src/merge.cpp
    src/merge.hpp
    src/misragries.cpp
    src/misragries.hpp
    src/parser.cpp
    src/parser.hpp
    src/reader.cpp
    src/reader.hpp
    src/record.hpp
    src/rolling_hash.cpp
    src/rolling_hash.hpp
    src/slidingwindow.cpp
    src/slidingwindow.hpp
    src/tokenizer.cpp
    src/tokenizer.hpp
    src/bottomk.cpp
    src/bottomk.hpp
)

find_package(nlohmann_json 3.12.0 REQUIRED)
find_package(zstd REQUIRED)

target_link_libraries(proj PRIVATE
    nlohmann_json::nlohmann_json
    zstd::libzstd
)
