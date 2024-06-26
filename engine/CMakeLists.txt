cmake_minimum_required(VERSION 3.27)
project(engine LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(Vulkan REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(eventpp CONFIG REQUIRED)

add_library(engine STATIC src/kat/engine.cpp src/kat/engine.hpp
        src/kat/window.cpp
        src/kat/window.hpp
        src/kat/render/render_pass.cpp
        src/kat/render/render_pass.hpp
        src/kat/render/command_recorder.cpp
        src/kat/render/command_recorder.hpp
        src/kat/vku.hpp
        src/kat/stack.hpp)
target_include_directories(engine PUBLIC src/)
target_link_libraries(engine PUBLIC Vulkan::Vulkan spdlog::spdlog glm::glm glfw eventpp::eventpp)
target_compile_definitions(engine PUBLIC -DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 -DKATENGINE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR} -DKATENGINE_VERSION_MINOR=${PROJECT_VERSION_MINOR} -DKATENGINE_VERSION_PATCH=${PROJECT_VERSION_PATCH})

target_compile_definitions(engine PUBLIC
        $<$<CONFIG:Debug>:KATENGINE_DEBUG>
#        $<$<CONFIG:Release>:KATENGINE_UNCHECKED_DESTROY>
#        $<$<CONFIG:RelWithDebInfo>:KATENGINE_UNCHECKED_DESTROY>
#        $<$<CONFIG:MinSizeRel>:KATENGINE_UNCHECKED_DESTROY>
)

add_library(kat::engine ALIAS engine)
