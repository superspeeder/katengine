#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <string>

namespace kat {
    struct WindowConfig {
        std::string title = "Window";
        glm::ivec2  size  = { 1200, 900 };
    };

    struct SwapchainConfig {
        vk::Format        format;
        vk::ColorSpaceKHR color_space;
        vk::Extent2D      extent;
    };

    struct EngineConfig {
        WindowConfig window_config{};
    };

    class Engine {
    public:
        explicit Engine(const EngineConfig &engine_config = {});

        ~Engine();

        void create_window(const WindowConfig &config);
        void create_instance();
        void select_physical_device();
        void create_surface();
        void select_queue_families();
        void create_device();
        void create_swapchain();
        void create_swapchain_image_views();

    private:
        GLFWwindow *m_window;

        vk::Instance       m_instance;
        vk::PhysicalDevice m_physical_device;
        vk::SurfaceKHR     m_surface;
        vk::Device         m_device;

        std::optional<uint32_t> m_transfer_family;
        std::optional<uint32_t> m_graphics_family;
        std::optional<uint32_t> m_present_family;
        std::optional<uint32_t> m_compute_family;

        bool m_has_gt_family;
        bool m_has_gtp_family;
        bool m_has_gtpc_family;

        vk::Queue        m_transfer_queue;
        vk::Queue        m_graphics_queue;
        vk::Queue        m_present_queue;
        vk::Queue        m_compute_queue;
        vk::SwapchainKHR m_swapchain;
        SwapchainConfig  m_swapchain_config{};

        std::vector<vk::Image>     m_swapchain_images;
        std::vector<vk::ImageView> m_swapchain_image_views;
    };
} // namespace kat
