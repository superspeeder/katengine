#pragma once

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

namespace kat {

    constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    template<typename T>
    using FrameSet = std::array<T, MAX_FRAMES_IN_FLIGHT>;

    struct WindowOptions {
        bool vsync = false;
        bool resizable = false;
    };


    /**
     * Important synchronization objects to ensure frames are rendered properly.
     *
     * Any rendering you do to the frame should wait on the imageAvailableSemaphore.
     *
     * The final rendering operation you do before you are ready to submit should signal the renderFinishedSemaphore.
     *
     * Your last rendering queue submission should also signal the inFlightFence that you are done rendering this frame (and no longer need the related resources).
     */
    struct FrameSyncResources {
        vk::Semaphore imageAvailableSemaphore;
        vk::Semaphore renderFinishedSemaphore;
        vk::Fence inFlightFence;

        FrameSyncResources();
        ~FrameSyncResources();

        FrameSyncResources(const FrameSyncResources&) = delete;
        FrameSyncResources& operator=(const FrameSyncResources&) = delete;
    };

    struct WindowFrameResources {
        vk::Image image;
        vk::ImageView imageView;
        uint32_t imageIndex;

        const FrameSyncResources* sync;
    };

    class Window {
        Window(const std::string &title, const vk::Extent2D &size, const WindowOptions &options, size_t id);

      public:
        static std::tuple<size_t, Window *> create(const std::string &title, const vk::Extent2D &size, const WindowOptions &options = {});
        static void destroy(size_t id);

        ~Window();

        [[nodiscard]] bool isOpen() const;
        void setClosed(bool closed) const;

        [[nodiscard]] vk::SurfaceKHR getSurface() const;

        void recreateSwapchain();

        [[nodiscard]] vk::Image getImage(uint32_t index) const;

        [[nodiscard]] uint32_t getImageCount() const noexcept;

        [[nodiscard]] const std::vector<vk::Image> &getImages() const;

        [[nodiscard]] vk::SurfaceFormatKHR getSurfaceFormat() const noexcept;

        [[nodiscard]] vk::PresentModeKHR getPresentMode() const noexcept;

        [[nodiscard]] vk::Extent2D getCurrentExtent() const noexcept;

        [[nodiscard]] const WindowFrameResources& getCurrentFrameResources() const;


        /**
         * Acquire the next image from the swapchain.
         *
         * Call getCurrentFrameResources() to get the current frame resources.
         * This function can fail, and will return false in that event.
         *
         * @return Whether or not the acquire was successful. If return value is false, skip the frame (failure will not reset the fence).
         */
        bool acquireFrame();

        void nextFrame();

        [[nodiscard]] const vk::SwapchainKHR &getSwapchain() const noexcept;

      private:
        size_t m_Id;
        GLFWwindow *m_Window;
        vk::SurfaceKHR m_Surface;

        bool m_EnableVsync;

        vk::SurfaceFormatKHR m_SwapchainFormat;
        vk::PresentModeKHR m_PresentMode;

        vk::SwapchainKHR m_Swapchain;
        vk::Extent2D m_CurrentExtent;
        std::vector<vk::Image> m_Images;
        std::vector<vk::ImageView> m_ImageViews;

        FrameSet<FrameSyncResources> m_SyncResources;

        WindowFrameResources m_CurrentFrameResources;

        uint32_t m_CurrentFrame = 0;
    };
} // namespace kat
