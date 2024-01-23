cmake_minimum_required(VERSION 3.26)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Stb REQUIRED)

set(CMAKE_CXX_STANDARD 23)

add_library(katengine src/kat/engine.cpp src/kat/engine.hpp
        src/kat/command_recorder.cpp
        src/kat/command_recorder.hpp
        src/kat/app.cpp
        src/kat/app.hpp
        src/kat/render_pipeline.cpp
        src/kat/render_pipeline.hpp)
target_include_directories(katengine PUBLIC src/ ${Stb_INCLUDE_DIR})
target_link_libraries(katengine PUBLIC glm::glm glfw Vulkan::Vulkan spdlog::spdlog)

add_library(katengine::katengine ALIAS katengine)
