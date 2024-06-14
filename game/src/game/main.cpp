#include <kat/core.hpp>
#include <kat/os/window.hpp>
#include <kat/vulkan_wrapper/basics.hpp>
#include <kat/vulkan_wrapper/swapchain.hpp>

#include <memory>

void runApplication() {
    kat::os::WindowConfiguration windowConfig{};
    windowConfig.title = "Hello!";
    windowConfig.mode.mode = kat::os::WindowModeType::WINDOWED;
    windowConfig.mode.settings = kat::os::WindowedMode{.size = {800, 600}, .resizable = false};
    //    windowConfig.mode.mode = kat::os::WindowModeType::BORDERLESS_FULLSCREEN;
    //    windowConfig.mode.settings = kat::os::FullscreenMode{.monitorId = 1ull};

    std::shared_ptr<kat::os::Window> window = std::make_shared<kat::os::Window>(windowConfig);

    kat::vkw::ContextSettings contextSettings{};
    kat::vkw::initContext(contextSettings);

    spdlog::info("GPU: {}", kat::vkw::context->gpu().getProperties().deviceName.data());


    while (!window->shouldClose()) {
        kat::os::Window::pollEvents();
    }
}

KAT_ENTRYPOINT(::runApplication)
