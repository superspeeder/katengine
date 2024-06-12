cmake_minimum_required(VERSION 3.27)
project(engine LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(Vulkan REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(eventpp CONFIG REQUIRED)

add_library(engine STATIC src/kat/core.hpp src/kat/core.cpp)
target_include_directories(engine PUBLIC src/)
target_link_libraries(engine PUBLIC Vulkan::Vulkan spdlog::spdlog glm::glm glfw eventpp::eventpp)
target_compile_definitions(engine PUBLIC -DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 -DKATENGINE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR} -DKATENGINE_VERSION_MINOR=${PROJECT_VERSION_MINOR} -DKATENGINE_VERSION_PATCH=${PROJECT_VERSION_PATCH})

target_compile_definitions(engine PUBLIC
        $<$<CONFIG:Debug>:KATENGINE_DEBUG>
)

add_library(kat::engine ALIAS engine)
