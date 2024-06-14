#pragma once

#include <string>
#include <optional>
#include <variant>

#include <glm/glm.hpp>

#include "kat/utils.hpp"

#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace kat::os {

    enum class WindowModeType {
        WINDOWED,
        BORDERLESS_FULLSCREEN,
        EXCLUSIVE_FULLSCREEN,
    };

    struct WindowedMode {
        glm::uvec2 size;
        std::optional<glm::ivec2> position;
        bool resizable = false;
        bool decorated = true;
        bool floating = false;
        bool maximized = false;
    };

    using MonitorId = std::variant<size_t /* TODO: non-positional monitor identification */>;

    struct FullscreenMode {
        MonitorId monitorId;
    };

    struct WindowMode {
        WindowModeType mode;
        std::variant<WindowedMode, FullscreenMode> settings;
    };

    struct WindowConfiguration {
        std::string title;
        WindowMode mode;
    };

    class Window {
      public:
        explicit Window(const WindowConfiguration& configuration);
        ~Window() = default;

        Window(Window&&) = default;
        Window& operator=(Window&&) = default;

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        [[nodiscard]] bool shouldClose() const;
        void setShouldClose(bool shouldClose) const;

        static void pollEvents();

        [[nodiscard]] const vk::SurfaceKHR& createVulkanSurface();

      private:

        GLFWwindow* m_Window;
        vk::SurfaceKHR m_Surface;
    };

} // namespace kat::os
