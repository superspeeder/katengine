#pragma once

#include "kat/engine.hpp"

namespace kat {

    class Window {
      public:
        Window(const std::string& title, const vk::Extent2D &size);
        ~Window();

        bool isOpen() const;
        void setClosed(bool closed) const;

        vk::SurfaceKHR getSurface() const;


      private:
        GLFWwindow* m_Window;
        vk::SurfaceKHR m_Surface;
    };

}// namespace kat
