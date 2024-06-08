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

    window->setWindowHandler(std::make_shared<WindowHandler>(window));

    kat::run();

    kat::terminate();
    return EXIT_SUCCESS;
}

WindowHandler::WindowHandler(kat::Window *window) : m_Window(window), kat::BaseWindowHandler() {
}

void WindowHandler::onRender(const std::shared_ptr<kat::Window> &window, const kat::WindowFrameResources &resources) {
    BaseWindowHandler::onRender(window, resources); // only calls this because we don't do our own thing yet.
}

