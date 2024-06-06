#include "window.hpp"

namespace kat {
    Window::Window(const std::string &title, const vk::Extent2D &size) {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, false);

        m_Window = glfwCreateWindow(size.width, size.height, title.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(m_Window, this);

        VkSurfaceKHR s;
        glfwCreateWindowSurface(globalState->instance, m_Window, nullptr, &s);
        m_Surface = s;

        globalState->activeWindows.insert(m_Window);
    }

    Window::~Window() {
        globalState->activeWindows.erase(m_Window);
        glfwDestroyWindow(m_Window);
    }
}// namespace kat