cmake_minimum_required(VERSION 3.10)

project(WordleSutom LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter")

add_executable(WordleSutom
    Application.cpp
)

target_link_libraries(WordleSutom PUBLIC tbb)
