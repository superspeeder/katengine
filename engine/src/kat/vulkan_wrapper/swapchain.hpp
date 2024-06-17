#pragma once

#include "basics.hpp"

#include "kat/os/window.hpp"

#include <memory>
#include <functional>

namespace kat::vkw {

    struct SwapchainConfiguration {
        vk::Format format;
        vk::ColorSpaceKHR colorSpace;
        vk::Extent2D extent;
        vk::PresentModeKHR presentMode;
    };

    struct SwapchainPreferences {
        std::vector<vk::PresentModeKHR> presentModePreferences;             // after any modes on this list, FIFO is used (fifo being present on this list will prevent any lower scoring modes from being available.
        std::optional<vk::PresentModeKHR> forcedPresentMode = std::nullopt; // if this isn't std::nullopt, then the swapchain will skip searching for valid present modes and just use this. If not available, the creation of the swapchain will throw an exception. If you only want fifo, this might be a good place to put it, as that is 100% available.
    };

    struct CurrentFrameInfo {
        vk::Image image;
        vk::ImageView imageView;
        uint32_t imageIndex;
        uint32_t currentFrame;
        vk::Fence inFlightFence;
        vk::Semaphore imageAvailableSemaphore;
        vk::Semaphore renderFinishedSemaphore;
    };

    inline const SwapchainPreferences SWAPCHAIN_PREFERENCES_DEFAULT_NO_VSYNC = {.presentModePreferences = {vk::PresentModeKHR::eImmediate, vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eFifoRelaxed}};
    inline const SwapchainPreferences SWAPCHAIN_PREFERENCES_DEFAULT_VSYNC = {.presentModePreferences = {vk::PresentModeKHR::eMailbox}};
    inline const SwapchainPreferences SWAPCHAIN_PREFERENCES_DEFAULT_FIFO_ONLY = {.forcedPresentMode = vk::PresentModeKHR::eFifo};

    class Swapchain final {
      public:
        static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

        Swapchain(const std::shared_ptr<kat::os::Window> &window, const SwapchainPreferences &preferences = SWAPCHAIN_PREFERENCES_DEFAULT_NO_VSYNC);
        ~Swapchain();

        [[nodiscard]] inline const SwapchainConfiguration &configuration() const noexcept { return m_Configuration; };

        [[nodiscard]] inline const vk::SwapchainKHR &swapchain() const noexcept { return m_Swapchain; };

        [[nodiscard]] inline const vk::SurfaceKHR &attachedSurface() const noexcept { return m_AttachedSurface; };

        [[nodiscard]] inline const CurrentFrameInfo &currentFrameInfo() const noexcept { return m_CurrentFrameInfo; };

        [[nodiscard]] inline const std::vector<vk::Image> &images() const noexcept { return m_Images; };

        [[nodiscard]] inline const std::vector<vk::ImageView> &imageViews() const noexcept { return m_ImageViews; }

        [[nodiscard]] inline const uint32_t &currentFrame() const noexcept { return m_CurrentFrame; };

        [[nodiscard]] const CurrentFrameInfo &beginPresentation();

        void endPresentation();

        void recreateSwapchain();

      private:
        vk::SurfaceKHR m_AttachedSurface;
        vk::SwapchainKHR m_Swapchain;
        std::shared_ptr<kat::os::Window> m_AttachedWindow;

        SwapchainConfiguration m_Configuration;

        CurrentFrameInfo m_CurrentFrameInfo;

        std::vector<vk::Image> m_Images;
        std::vector<vk::ImageView> m_ImageViews;

        std::vector<vk::Fence> m_InFlightFences;
        std::vector<vk::Semaphore> m_ImageAvailableSemaphores;
        std::vector<vk::Semaphore> m_RenderFinishedSemaphores;

        uint32_t m_CurrentFrame = 0;

        SwapchainPreferences m_Preferences;
    };

    struct FrameCommandSet {
        std::array<vk::CommandBuffer, Swapchain::MAX_FRAMES_IN_FLIGHT> commandBuffers;
        vk::CommandPool pool;

        inline const vk::CommandBuffer& operator[](size_t index) const {
            return get(index);
        };

        inline const vk::CommandBuffer& operator[](const CurrentFrameInfo& cfi) const {
            return get(cfi.currentFrame);
        };

        inline const vk::CommandBuffer& get(size_t index) const {
            assert(index < Swapchain::MAX_FRAMES_IN_FLIGHT);
            return commandBuffers[index];
        };

        inline const vk::CommandBuffer& get(const CurrentFrameInfo& cfi) const {
            return get(cfi.currentFrame);
        };

        FrameCommandSet(const vk::CommandPool& commandPool);
        ~FrameCommandSet();

        // disallow copy
        FrameCommandSet(const FrameCommandSet&) = delete;
        FrameCommandSet& operator=(const FrameCommandSet&) = delete;

        // allow move
        FrameCommandSet(FrameCommandSet&&) = default;
        FrameCommandSet& operator=(FrameCommandSet&&) = default;

    };


    class EasySwapchain final {
      public:
        EasySwapchain(const std::shared_ptr<kat::os::Window> &window, const SwapchainPreferences &preferences = SWAPCHAIN_PREFERENCES_DEFAULT_NO_VSYNC);
        ~EasySwapchain();

        void renderAndPresent(const std::function<void(const vk::CommandBuffer&, const CurrentFrameInfo&)>& renderFunction);

      private:
        std::unique_ptr<Swapchain> m_Swapchain;
        vk::CommandPool m_RenderPool;
        std::unique_ptr<FrameCommandSet> m_FrameCommandSet;
    };

} // namespace kat::vkw
