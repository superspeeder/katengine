#pragma once

#include <stdexcept>
#include <cstdlib>

#include <spdlog/spdlog.h>

namespace kat {
    void initSubsystems();
    void initLibraries();

    void terminateSubsystems();
    void terminateLibraries();

    void init();
    void terminate();
}


#define KAT_ENTRYPOINT(f) int main() { try { ::kat::init(); f(); ::kat::terminate(); } catch (const std::exception &e) { spdlog::critical(e.what()); return EXIT_FAILURE; } return EXIT_SUCCESS; }