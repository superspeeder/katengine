#include "window.hpp"
#include "kat/engine.hpp"

namespace kat {
    FrameSyncResources::FrameSyncResources() {
        imageAvailableSemaphore = vku::createSemaphore();
        renderFinishedSemaphore = vku::createSemaphore();
        inFlightFence = vku::createFenceSignaled();
    }

    FrameSyncResources::~FrameSyncResources() {
        vku::waitFence(inFlightFence);
        kat::destroy(inFlightFence);
        kat::destroy(imageAvailableSemaphore);
        kat::destroy(renderFinishedSemaphore);
    }

    vk::PresentModeKHR selectPresentMode(const std::vector<vk::PresentModeKHR> &presentModes, bool vsync) {
        bool foundImmediate = false;
        bool foundFifoRelaxed = false;
        bool foundMailbox = false;

        for (const auto &pm: presentModes) {
            if (pm == vk::PresentModeKHR::eMailbox) foundMailbox = true;
            if (pm == vk::PresentModeKHR::eFifoRelaxed) foundFifoRelaxed = true;
            if (pm == vk::PresentModeKHR::eImmediate) foundImmediate = true;
        }

        if (!vsync) {
            if (foundImmediate) return vk::PresentModeKHR::eImmediate;
            if (foundMailbox) return vk::PresentModeKHR::eMailbox;
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

        spdlog::info("Present Mode: {}", vk::to_string(m_PresentMode));

        recreateSwapchain();

        m_WindowHandler = std::make_shared<kat::BaseWindowHandler>(); // default window handler impl
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
                vku::waitFence(s.inFlightFence);
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

        vku::waitFence(syncResources.inFlightFence);

        auto r = globalState->device.acquireNextImageKHR(m_Swapchain, UINT64_MAX, syncResources.imageAvailableSemaphore);
        if (r.result == vk::Result::eErrorOutOfDateKHR) {
            recreateSwapchain();
            return false; // frame is skipped.
        }

        vku::resetFence(syncResources.inFlightFence);

        m_CurrentFrameResources.imageIndex = r.value;
        m_CurrentFrameResources.image = m_Images[r.value];
        m_CurrentFrameResources.imageView = m_ImageViews[r.value];

        return true;
    }

    void Window::nextFrame() {
        m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    vk::Image Window::getImage(uint32_t index) const {
        return getCurrentFrameResources().image;
    }

    uint32_t Window::getImageCount() const noexcept {
        return m_Images.size();
    }

    const std::vector<vk::Image> &Window::getImages() const {
        return m_Images;
    }

    vk::SurfaceFormatKHR Window::getSurfaceFormat() const noexcept {
        return m_SwapchainFormat;
    }

    vk::PresentModeKHR Window::getPresentMode() const noexcept {
        return m_PresentMode;
    }

    vk::Extent2D Window::getCurrentExtent() const noexcept {
        return m_CurrentExtent;
    }

    const vk::SwapchainKHR &Window::getSwapchain() const noexcept {
        return m_Swapchain;
    }

    const WindowFrameResources &Window::getCurrentFrameResources() const {
        return m_CurrentFrameResources;
    }

    const std::shared_ptr<BaseWindowHandler> &Window::getWindowHandler() const {
        return m_WindowHandler;
    }

    const std::vector<vk::ImageView> &Window::getImageViews() const {
        return m_ImageViews;
    }

    void BaseWindowHandler::onRender(const std::shared_ptr<Window> &window, const WindowFrameResources &resources) {
        vku::OTCSync otcs{};
        otcs.wait = resources.sync->imageAvailableSemaphore;
        otcs.signal = resources.sync->renderFinishedSemaphore;

        vku::otc([&](const vk::CommandBuffer& cmd) {
            vk::ImageMemoryBarrier2 imb{};
            imb.image = resources.image;
            imb.srcAccessMask = vk::AccessFlagBits2::eNone;
            imb.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
            imb.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
            imb.dstStageMask = vk::PipelineStageFlagBits2::eClear;
            imb.srcQueueFamilyIndex = globalState->mainFamily;
            imb.dstQueueFamilyIndex = globalState->mainFamily;
            imb.oldLayout = vk::ImageLayout::eUndefined;
            imb.newLayout = vk::ImageLayout::eTransferDstOptimal;
            imb.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

            float n = (sinf(float(glfwGetTime())) + 1.0f) / 2.0f;

            vk::ClearColorValue clearValue{n, 0.0f, 0.0f, 1.0f};
            vk::ImageSubresourceRange range{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};


            cmd.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, imb));
            cmd.clearColorImage(resources.image, vk::ImageLayout::eTransferDstOptimal, clearValue, range);

            vk::ImageMemoryBarrier2 imb2{};
            imb2.image = resources.image;
            imb2.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
            imb2.dstAccessMask = vk::AccessFlagBits2::eNone;
            imb2.srcStageMask = vk::PipelineStageFlagBits2::eClear;
            imb2.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
            imb2.srcQueueFamilyIndex = globalState->mainFamily;
            imb2.dstQueueFamilyIndex = globalState->mainFamily;
            imb2.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            imb2.newLayout = vk::ImageLayout::ePresentSrcKHR;
            imb2.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

            cmd.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, imb2));
        }, resources.sync->inFlightFence, otcs, window);
    }

    BaseWindowHandler::BaseWindowHandler() {
    }
} // namespace kat