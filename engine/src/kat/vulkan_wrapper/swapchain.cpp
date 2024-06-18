#include "swapchain.hpp"

namespace kat::vkw {
    Swapchain::Swapchain(const std::shared_ptr<kat::os::Window> &window, const SwapchainPreferences &preferences) : m_Preferences(preferences), m_AttachedWindow(window) {
        m_AttachedSurface = window->createVulkanSurface();

        m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0 ; i < MAX_FRAMES_IN_FLIGHT ; i++) {
            m_InFlightFences[i] = createSignaledFence();
            m_ImageAvailableSemaphores[i] = createSemaphore();
            m_RenderFinishedSemaphores[i] = createSemaphore();
        }

        recreateSwapchain();

        // TODO;
    }

    Swapchain::~Swapchain() {
        for (const auto& iv : m_ImageViews) {
            destroy(iv);
        }

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            destroy(m_InFlightFences[i], m_ImageAvailableSemaphores[i], m_RenderFinishedSemaphores[i]);
        }

        vkw::destroy(m_Swapchain);

        // TODO;
    }

    const CurrentFrameInfo &Swapchain::beginPresentation() {
        vkw::waitFence(m_InFlightFences[m_CurrentFrame]);
        vkw::resetFence(m_InFlightFences[m_CurrentFrame]);

        auto imageIndexResult = context->device().acquireNextImageKHR(m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], nullptr);

        uint32_t imageIndex;
        if (imageIndexResult.result == vk::Result::eErrorOutOfDateKHR) {
            recreateSwapchain();
            imageIndex = context->device().acquireNextImageKHR(m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], nullptr).value;
        } else {
            imageIndex = imageIndexResult.value;
        }

        m_CurrentFrameInfo.image = m_Images[imageIndex];
        m_CurrentFrameInfo.imageView = m_ImageViews[imageIndex];
        m_CurrentFrameInfo.imageIndex = imageIndex;
        m_CurrentFrameInfo.currentFrame = m_CurrentFrame;
        m_CurrentFrameInfo.inFlightFence = m_InFlightFences[m_CurrentFrame];
        m_CurrentFrameInfo.imageAvailableSemaphore = m_ImageAvailableSemaphores[m_CurrentFrame];
        m_CurrentFrameInfo.renderFinishedSemaphore = m_RenderFinishedSemaphores[m_CurrentFrame];

        return m_CurrentFrameInfo;
    }

    void Swapchain::endPresentation() {
        vk::PresentInfoKHR presentInfo{};
        presentInfo.setSwapchains(m_Swapchain);
        presentInfo.setImageIndices(m_CurrentFrameInfo.imageIndex);
        presentInfo.setWaitSemaphores(m_CurrentFrameInfo.renderFinishedSemaphore);

        try {
            auto _ = context->mainQueue().presentKHR(presentInfo);
        } catch (const vk::OutOfDateKHRError &_) {
            recreateSwapchain();
        }

        m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void Swapchain::recreateSwapchain() {
        if (m_Swapchain) {
            context->device().waitIdle(); // everything must stop first
            for (uint32_t i = 0; i < m_Images.size(); i++) {
                destroy(m_ImageViews[i]);
            }
        }

        vk::SurfaceCapabilitiesKHR caps = context->gpu().getSurfaceCapabilitiesKHR(m_AttachedSurface);
        auto surfaceFormats = context->gpu().getSurfaceFormatsKHR(m_AttachedSurface);
        auto presentModes = context->gpu().getSurfacePresentModesKHR(m_AttachedSurface);

        vk::SurfaceFormatKHR sFormat = surfaceFormats[0];
        for (const auto &sf: surfaceFormats) {
            if (sf.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                if (sf.format == vk::Format::eR8G8B8A8Srgb) {
                    sFormat = sf;
                } else if (sf.format == vk::Format::eB8G8R8A8Srgb) {
                    sFormat = sf;
                    break;
                }
            }
        }

        vk::SwapchainCreateInfoKHR createInfo{};
        createInfo.surface = m_AttachedSurface;
        createInfo.minImageCount = caps.minImageCount + 1;
        if (caps.maxImageCount > 0 && createInfo.minImageCount > caps.maxImageCount) createInfo.minImageCount = caps.maxImageCount;
        createInfo.imageFormat = sFormat.format;
        createInfo.imageColorSpace = sFormat.colorSpace;
        createInfo.imageExtent = caps.maxImageExtent;
        if (createInfo.imageExtent.height == UINT32_MAX) {
            vk::Extent2D windowExtent = m_AttachedWindow->getInnerExtent();

            createInfo.imageExtent = vk::Extent2D{
                    std::clamp(windowExtent.width, caps.minImageExtent.width, caps.maxImageExtent.width),
                    std::clamp(windowExtent.height, caps.minImageExtent.height, caps.maxImageExtent.height),
            };
        }

        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment;
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
        createInfo.preTransform = caps.currentTransform;
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

        if (m_Preferences.forcedPresentMode.has_value()) {
            createInfo.presentMode = m_Preferences.forcedPresentMode.value();
        } else if (!m_Preferences.presentModePreferences.empty()) {
            createInfo.presentMode = vk::PresentModeKHR::eFifo;
            size_t dist = SIZE_MAX;
            for (const auto &pm: presentModes) {
                auto it = std::find(m_Preferences.presentModePreferences.begin(), m_Preferences.presentModePreferences.end(), pm);
                if (it != m_Preferences.presentModePreferences.end()) {
                    size_t d = std::distance(m_Preferences.presentModePreferences.begin(), it);
                    if (d < dist) {
                        dist = d;
                        createInfo.presentMode = *it;
                    }
                }
            }
        } else {
            createInfo.presentMode = vk::PresentModeKHR::eFifo;
        }

        createInfo.clipped = true;
        createInfo.oldSwapchain = m_Swapchain;

        m_Configuration.format = createInfo.imageFormat;
        m_Configuration.colorSpace = createInfo.imageColorSpace;
        m_Configuration.extent = createInfo.imageExtent;
        m_Configuration.presentMode = createInfo.presentMode;

        m_Swapchain = context->device().createSwapchainKHR(createInfo, allocator);
        if (createInfo.oldSwapchain) {
            destroy(createInfo.oldSwapchain);
        }

        m_Images = context->device().getSwapchainImagesKHR(m_Swapchain);

        m_ImageViews.resize(m_Images.size());
        for (size_t i = 0; i < m_Images.size(); i++) {
            m_ImageViews[i] = context->device().createImageView(vk::ImageViewCreateInfo({}, m_Images[i], vk::ImageViewType::e2D, m_Configuration.format, vk::ComponentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
        }
    }

    FrameCommandSet::FrameCommandSet(const vk::CommandPool &commandPool) : pool(commandPool) {
        auto cmds = context->device().allocateCommandBuffers(vk::CommandBufferAllocateInfo(commandPool, vk::CommandBufferLevel::ePrimary, Swapchain::MAX_FRAMES_IN_FLIGHT));
        assert(cmds.size() == Swapchain::MAX_FRAMES_IN_FLIGHT);
        std::memcpy(commandBuffers.data(), cmds.data(), sizeof(vk::CommandBuffer) * Swapchain::MAX_FRAMES_IN_FLIGHT);
    }

    FrameCommandSet::~FrameCommandSet() {
        context->device().freeCommandBuffers(pool, commandBuffers);
    }

    EasySwapchain::EasySwapchain(const std::shared_ptr<kat::os::Window> &window, const SwapchainPreferences &preferences) {
        m_Swapchain = std::make_unique<Swapchain>(window, preferences);
        m_RenderPool = context->device().createCommandPool(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, context->mainQueueFamily()));
        m_FrameCommandSet = std::make_unique<FrameCommandSet>(m_RenderPool);
    }

    EasySwapchain::~EasySwapchain() {
        m_FrameCommandSet.reset();
        destroy(m_RenderPool);
    }

    void EasySwapchain::renderAndPresent(const std::function<void(const vk::CommandBuffer &, const CurrentFrameInfo &)> &renderFunction) {
        auto cfi = m_Swapchain->beginPresentation();
        vk::CommandBuffer cmd = m_FrameCommandSet->get(cfi);

        cmd.reset();
        cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        renderFunction(cmd, cfi);
        cmd.end();

        vk::SubmitInfo si{};

        std::array<vk::PipelineStageFlags, 1> waitDstStage = {vk::PipelineStageFlagBits::eTopOfPipe};
        si.setWaitSemaphores(cfi.imageAvailableSemaphore);
        si.setSignalSemaphores(cfi.renderFinishedSemaphore);
        si.setCommandBuffers(cmd);
        si.setWaitDstStageMask(waitDstStage);
        context->mainQueue().submit(si, cfi.inFlightFence);

        m_Swapchain->endPresentation();
    }
} // namespace kat::vkw
