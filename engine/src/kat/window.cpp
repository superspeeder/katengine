#include "window.hpp"
#include "kat/engine.hpp"

namespace kat {
    FrameSyncResources::FrameSyncResources() {
        imageAvailableSemaphore = kat::vku::createSemaphore();
        renderFinishedSemaphore = kat::vku::createSemaphore();
        inFlightFence = kat::vku::createFenceSignaled();
    }

    FrameSyncResources::~FrameSyncResources() {
        kat::vku::waitFence(inFlightFence);
        kat::destroy(inFlightFence);
        kat::destroy(imageAvailableSemaphore);
        kat::destroy(renderFinishedSemaphore);
    }

    vk::PresentModeKHR selectPresentMode(const std::vector<vk::PresentModeKHR> &presentModes, bool vsync) {
        bool foundImmediate = false;
        bool foundFifoRelaxed = false;

        for (const auto &pm: presentModes) {
            if (pm == vk::PresentModeKHR::eMailbox) return pm;
            if (pm == vk::PresentModeKHR::eFifoRelaxed) foundFifoRelaxed = true;
            if (pm == vk::PresentModeKHR::eImmediate) foundImmediate = true;
        }

        if (!vsync) {
            if (foundImmediate) return vk::PresentModeKHR::eImmediate;
            if (foundFifoRelaxed) return vk::PresentModeKHR::eFifoRelaxed;
        }

        return vk::PresentModeKHR::eFifo;
    }

    vk::SurfaceFormatKHR selectSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &surfaceFormats) {
        bool rgbasrgb = false;
        bool bgraunorm = false;
        bool rgbaunorm = false;

        for (const auto &sf: surfaceFormats) {
            if (sf.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                if (sf.format == vk::Format::eB8G8R8A8Srgb) return sf;
                if (sf.format == vk::Format::eR8G8B8A8Srgb) rgbasrgb = true;
                if (sf.format == vk::Format::eB8G8R8A8Unorm) bgraunorm = true;
                if (sf.format == vk::Format::eR8G8B8A8Unorm) rgbaunorm = true;
            }
        }

        if (rgbasrgb) return {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear};
        if (bgraunorm) return {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
        if (rgbaunorm) return {vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
        return surfaceFormats[0];
    }

    Window::Window(const std::string &title, const vk::Extent2D &size, const WindowOptions &options, size_t id) : m_Id(id), m_EnableVsync(options.vsync) {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, options.resizable);

        m_Window = glfwCreateWindow(size.width, size.height, title.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(m_Window, this);

        VkSurfaceKHR s;
        glfwCreateWindowSurface(globalState->instance, m_Window, nullptr, &s);
        m_Surface = s;

        globalState->activeWindowCount++;

        glfwSetWindowCloseCallback(m_Window, +[](GLFWwindow *window_) {
            auto* window = static_cast<Window *>(glfwGetWindowUserPointer(window_));

            // TODO: cancelable on close event.

            // following code should only be run if window should actually be closed
            kat::Window::destroy(window->m_Id); });

        auto formats = globalState->physicalDevice.getSurfaceFormatsKHR(m_Surface);
        auto presentModes = globalState->physicalDevice.getSurfacePresentModesKHR(m_Surface);

        m_SwapchainFormat = selectSurfaceFormat(formats);
        m_PresentMode = selectPresentMode(presentModes, m_EnableVsync);

        recreateSwapchain();
    }

    Window::~Window() {
        globalState->activeWindowCount--;
        // in case this somehow happens earlier than it should;
        globalState->activeWindows.erase(m_Id);

        for (const auto &iv: m_ImageViews) {
            kat::destroy(iv);
        }

        kat::destroy(m_Swapchain);


        kat::destroy(m_Surface);

        glfwDestroyWindow(m_Window);
    }

    std::tuple<size_t, Window *> Window::create(const std::string &title, const vk::Extent2D &size, const WindowOptions &options) {
        size_t id = globalState->nextWindowId++;
        globalState->activeWindows[id] = std::unique_ptr<Window>(new Window(title, size, options, id));
        return std::make_tuple(id, globalState->activeWindows[id].get());
    }

    void Window::destroy(size_t id) {
        globalState->activeWindows.erase(id);
    }

    bool Window::isOpen() const {
        return !glfwWindowShouldClose(m_Window);
    }

    void Window::setClosed(bool closed) const {
        glfwSetWindowShouldClose(m_Window, closed);
    }

    vk::SurfaceKHR Window::getSurface() const {
        return m_Surface;
    }

    void Window::recreateSwapchain() {
        vk::SwapchainKHR oldSwapchain = m_Swapchain;

        auto capabilities = globalState->physicalDevice.getSurfaceCapabilitiesKHR(m_Surface);

        m_CurrentExtent = capabilities.currentExtent;
        if (m_CurrentExtent.height == UINT32_MAX) {
            int w, h;
            glfwGetFramebufferSize(m_Window, &w, &h);
            m_CurrentExtent.width = std::clamp(static_cast<uint32_t>(w), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            m_CurrentExtent.height = std::clamp(static_cast<uint32_t>(h), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }

        uint32_t minImageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && minImageCount > capabilities.maxImageCount) {
            minImageCount = capabilities.maxImageCount;
        }

        auto sci = vk::SwapchainCreateInfoKHR()
                           .setSurface(m_Surface)
                           .setMinImageCount(minImageCount)
                           .setImageFormat(m_SwapchainFormat.format)
                           .setImageColorSpace(m_SwapchainFormat.colorSpace)
                           .setImageExtent(m_CurrentExtent)
                           .setImageArrayLayers(1)
                           .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst)
                           .setImageSharingMode(vk::SharingMode::eExclusive)
                           .setPreTransform(capabilities.currentTransform)
                           .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                           .setPresentMode(m_PresentMode)
                           .setClipped(true)
                           .setOldSwapchain(oldSwapchain);

        if (oldSwapchain) {
            // wait for all in flight frames to complete rendering so we don't pull resources out from under them.
            for (const auto &s: m_SyncResources) {
                kat::vku::waitFence(s.inFlightFence);
            }

            for (const auto &iv: m_ImageViews) {
                kat::destroy(iv);
            }

            kat::destroy(oldSwapchain);
        }

        m_Swapchain = globalState->device.createSwapchainKHR(sci);

        m_Images = globalState->device.getSwapchainImagesKHR(m_Swapchain);

        m_ImageViews.resize(m_Images.size());
        for (size_t i = 0; i < m_Images.size(); i++) {
            m_ImageViews[i] = globalState->device.createImageView(vk::ImageViewCreateInfo(
                    {}, m_Images[i], vk::ImageViewType::e2D, m_SwapchainFormat.format,
                    vk::ComponentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA),
                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
        }
    }

    bool Window::acquireFrame() {
        const auto &syncResources = m_SyncResources[m_CurrentFrame];

        m_CurrentFrameResources.sync = &m_SyncResources[m_CurrentFrame];

        kat::vku::waitFence(syncResources.inFlightFence);

        auto r = globalState->device.acquireNextImageKHR(m_Swapchain, UINT64_MAX, syncResources.imageAvailableSemaphore);
        if (r.result == vk::Result::eErrorOutOfDateKHR) {
            recreateSwapchain();
            return false; // frame is skipped.
        }

        kat::vku::resetFence(syncResources.inFlightFence);

        m_CurrentFrameResources.imageIndex = r.value;
        m_CurrentFrameResources.image = m_Images[r.value];
        m_CurrentFrameResources.imageView = m_ImageViews[r.value];

        return true;
    }

    const WindowFrameResources &Window::getCurrentFrameResources() const {
        return m_CurrentFrameResources;
    }

    void Window::nextFrame() {
        m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    const vk::SwapchainKHR &Window::getSwapchain() const noexcept {
        return m_Swapchain;
    }

} // namespace kat