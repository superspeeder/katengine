cmake_minimum_required(VERSION 3.27)
project(game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)


add_executable(game src/game/main.cpp src/game/main.hpp)
target_include_directories(game PUBLIC src/)
target_link_libraries(game PRIVATE kat::engine)

