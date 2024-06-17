#include "window.hpp"

#include <stdexcept>

#include <vector>

#include <spdlog/spdlog.h>

#include "kat/vulkan_wrapper/basics.hpp"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <dwmapi.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif


namespace kat::os::u_ {
    using namespace winrt;
    using namespace Windows::UI::ViewManagement;

    inline bool IsColorLight(Windows::UI::Color& clr)
    {
        return (((5 * clr.G) + (2 * clr.R) + clr.B) > (8 * 128));
    }

    inline void fitTheme(GLFWwindow* window) {
        HWND hwnd = glfwGetWin32Window(window);
        auto settings = UISettings();

        auto foreground = settings.GetColorValue(UIColorType::Foreground);
        bool isDarkMode = static_cast<bool>(IsColorLight(foreground));

        BOOL value = isDarkMode;
        ::DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
        RedrawWindow(hwnd, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_INTERNALPAINT);
    }
}

#endif

namespace kat::os {
    namespace u_ {
        std::vector<GLFWmonitor*> getMonitors() {
            int count;
            GLFWmonitor** pMonitors = glfwGetMonitors(&count);
            return std::vector<GLFWmonitor*>(pMonitors, pMonitors + count); // NOLINT(*-return-braced-init-list)
        }

        GLFWmonitor* findMonitorById(const std::vector<GLFWmonitor*>& monitors, const MonitorId& id) {
            switch (id.index()) {
                case 0: {
                    size_t index = std::get<0>(id);
                    if (monitors.empty()) throw std::runtime_error("No monitors are available.");
                    if (index >= monitors.size()) {
                        spdlog::error("Monitor index {} is out of range. Defaulting to primary monitor.", index);
                        index = 0;
                    }

                    return monitors.at(index);
                }
                default: throw std::runtime_error("Bad monitor id");
            }
        }
    }

    Window::Window(const WindowConfiguration &configuration) {
        if (!glfwVulkanSupported()) {
            throw std::runtime_error("Vulkan is not supported");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        glm::ivec2 size;
        GLFWmonitor* monitor = nullptr;
        bool resizable = false;
        bool decorated = true;
        glm::ivec2 position = {GLFW_ANY_POSITION, GLFW_ANY_POSITION};

        switch (configuration.mode.mode) {
            case WindowModeType::WINDOWED: {
                auto mode = std::get<WindowedMode>(configuration.mode.settings);
                size = { mode.size.x, mode.size.y };
                resizable = mode.resizable;
                decorated = mode.decorated;
                glfwWindowHint(GLFW_FLOATING, mode.floating);
                glfwWindowHint(GLFW_MAXIMIZED, mode.maximized);
                if (mode.position.has_value()) {
                    position = mode.position.value();
                }
            } break;
            case WindowModeType::BORDERLESS_FULLSCREEN: {
                auto mode = std::get<FullscreenMode>(configuration.mode.settings);
                std::vector<GLFWmonitor*> monitors = u_::getMonitors();
                GLFWmonitor* m = u_::findMonitorById(monitors, mode.monitorId);
                const GLFWvidmode* vm = glfwGetVideoMode(m);
                size = { vm->width, vm->height };
                glfwGetMonitorPos(m, &position.x, &position.y);
                decorated = false;
            } break;
            case WindowModeType::EXCLUSIVE_FULLSCREEN: {
                auto mode = std::get<FullscreenMode>(configuration.mode.settings);
                std::vector<GLFWmonitor*> monitors = u_::getMonitors();
                monitor = u_::findMonitorById(monitors, mode.monitorId);
                const GLFWvidmode* vm = glfwGetVideoMode(monitor);
                size = { vm->width, vm->height };
            } break;
        }

        glfwWindowHint(GLFW_RESIZABLE, resizable);
        glfwWindowHint(GLFW_DECORATED, decorated);
        glfwWindowHint(GLFW_POSITION_X, position.x);
        glfwWindowHint(GLFW_POSITION_Y, position.y);

        m_Window = glfwCreateWindow(size.x, size.y, configuration.title.c_str(), monitor, nullptr);

#ifdef WIN32
        u_::fitTheme(m_Window);
#endif
    }

    bool Window::shouldClose() const {
        return glfwWindowShouldClose(m_Window);
    }

    void Window::setShouldClose(bool shouldClose) const {
        glfwSetWindowShouldClose(m_Window, shouldClose);
    }

    void Window::pollEvents() {
        glfwPollEvents();
    }

    const vk::SurfaceKHR &Window::createVulkanSurface() {
        VkSurfaceKHR surf;
        const vk::AllocationCallbacks* allocationCallbacks_ = vkw::allocator;
        const VkAllocationCallbacks* allocationCallbacks = nullptr;
        if (allocationCallbacks_) {
            allocationCallbacks = &(VkAllocationCallbacks&)*allocationCallbacks_;
        }

        glfwCreateWindowSurface(vkw::context->instance(), m_Window, allocationCallbacks, &surf);

        m_Surface = surf;
        return m_Surface;
    }

    vk::Extent2D Window::getInnerExtent() const {
        int w, h;
        glfwGetFramebufferSize(m_Window, &w, &h);
        return vk::Extent2D{static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
    }
} // namespace kat::os
