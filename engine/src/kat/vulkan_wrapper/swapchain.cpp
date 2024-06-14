#include "swapchain.hpp"

namespace kat::vkw {
    Swapchain::Swapchain(const std::shared_ptr<kat::os::Window> &window, const SwapchainPreferences &preferences) {
        m_AttachedSurface = window->createVulkanSurface();
        vk::SurfaceCapabilitiesKHR caps = kat::vkw::context->gpu().getSurfaceCapabilitiesKHR(m_AttachedSurface);

        vk::SwapchainCreateInfoKHR createInfo{};

        // TODO;
    }

    Swapchain::~Swapchain() {
        // TODO;
    }
} // namespace kat::vkw
