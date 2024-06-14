#include "kat/core.hpp"

#include "kat/os/window.hpp"
#include "kat/vulkan_wrapper/basics.hpp"

namespace kat {
    void initSubsystems() {
        kat::vkw::destroyContext();
    }

    void initLibraries() {
        glfwInit();
    }

    void terminateSubsystems() {
    }

    void terminateLibraries() {
        glfwTerminate();
    }

    void init() {
        initLibraries();
        initSubsystems();
    }

    void terminate() {
        terminateSubsystems();
        terminateLibraries();
    }
}
