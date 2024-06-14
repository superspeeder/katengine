#pragma once

#include "basics.hpp"

#include "kat/os/window.hpp"

namespace kat::vkw {

    struct SwapchainConfiguration {
        vk::Format format;
        vk::ColorSpaceKHR colorSpace;
        vk::Extent2D extent;
    };

    struct SwapchainPreferences {
        std::vector<vk::PresentModeKHR> presentModePreferences; // after any modes on this list, FIFO is used (fifo being present on this list will prevent any lower scoring modes from being available.
        std::optional<vk::PresentModeKHR> forcedPresentMode = std::nullopt; // if this isn't std::nullopt, then the swapchain will skip searching for valid present modes and just use this. If not available, the creation of the swapchain will throw an exception. If you only want fifo, this might be a good place to put it, as that is 100% available.
    };

    inline const SwapchainPreferences SWAPCHAIN_PREFERENCES_DEFAULT_NO_VSYNC = {.presentModePreferences = {vk::PresentModeKHR::eImmediate, vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eFifoRelaxed}};
    inline const SwapchainPreferences SWAPCHAIN_PREFERENCES_DEFAULT_VSYNC = {.presentModePreferences = {vk::PresentModeKHR::eMailbox}};
    inline const SwapchainPreferences SWAPCHAIN_PREFERENCES_DEFAULT_FIFO_ONLY = {.forcedPresentMode = vk::PresentModeKHR::eFifo};

    class Swapchain final {
      public:
        Swapchain(const std::shared_ptr<kat::os::Window>& window, const SwapchainPreferences& preferences = SWAPCHAIN_PREFERENCES_DEFAULT_NO_VSYNC);
        ~Swapchain();

      private:
        vk::SurfaceKHR m_AttachedSurface;
        vk::SwapchainKHR m_Swapchain;
    };

} // namespace kat::vkw
