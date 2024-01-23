cmake_minimum_required(VERSION 3.26)

set(CMAKE_CXX_STANDARD 23)

add_executable(sample src/sample/main.cpp
        src/sample/main.hpp)
target_include_directories(sample PRIVATE src/)
target_link_libraries(sample PRIVATE katengine::katengine)


