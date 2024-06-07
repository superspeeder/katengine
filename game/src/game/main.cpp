#include "game/main.hpp"

#include <cstdlib>
#include <iostream>

int main(int argc, char *argv[]) {
    kat::init();

    kat::setValidationLayersEnabled(true);
    //    kat::setApiDumpEnabled(true);

    kat::startup();

    kat::globalState->doRenderSetup = true;
    kat::globalState->isRenderSetupOnlyOperation = true;

    kat::Window *window;
    size_t windowId;
    std::tie(windowId, window) = kat::Window::create("Hello!", vk::Extent2D{800, 600}, kat::WindowOptions{false});

    kat::run();

    kat::terminate();
    return EXIT_SUCCESS;
}
