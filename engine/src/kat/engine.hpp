#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <memory>
#include <optional>
#include <string>

#include "command_recorder.hpp"

namespace kat {
    constexpr vk::ComponentMapping STANDARD_COMPONENT_MAPPING{ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                                                               vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };

    constexpr vk::ImageSubresourceRange color_subresource_range(uint32_t num_levels, uint32_t num_layers) {
        return { vk::ImageAspectFlagBits::eColor, 0, num_levels, 0, num_layers };
    }

    constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    struct WindowConfig {
        std::string title = "Window";
        glm::ivec2  size  = { 1200, 900 };
    };

    struct SwapchainConfig {
        vk::SurfaceFormatKHR surface_format;
        vk::Extent2D         extent;
        vk::PresentModeKHR   present_mode;
    };

    struct EngineConfig {
        WindowConfig window_config{};
    };

    class Engine : public std::enable_shared_from_this<Engine> {
        explicit Engine(const EngineConfig &engine_config);

        void                 create_window(const WindowConfig &config);
        void                 create_instance();
        void                 select_physical_device();
        void                 create_surface();
        void                 select_queue_families();
        void                 create_device();
        void                 create_swapchain();
        vk::SurfaceFormatKHR select_surface_format() const;
        vk::PresentModeKHR   select_present_mode() const;
        void                 create_command_buffers();
        void                 create_syncs();

    public:
        static inline std::shared_ptr<Engine> create(const EngineConfig &config = {}) {
            return std::shared_ptr<Engine>(new Engine(config));
        };

        ~Engine();

        [[nodiscard]] GLFWwindow                       *get_window() const { return m_window; }
        [[nodiscard]] const vk::Instance               &get_instance() const { return m_instance; }
        [[nodiscard]] const vk::PhysicalDevice         &get_physical_device() const { return m_physical_device; }
        [[nodiscard]] const vk::SurfaceKHR             &get_surface() const { return m_surface; }
        [[nodiscard]] const vk::Device                 &get_device() const { return m_device; }
        [[nodiscard]] const std::optional<uint32_t>    &get_transfer_family() const { return m_transfer_family; }
        [[nodiscard]] const std::optional<uint32_t>    &get_graphics_family() const { return m_graphics_family; }
        [[nodiscard]] const std::optional<uint32_t>    &get_present_family() const { return m_present_family; }
        [[nodiscard]] const std::optional<uint32_t>    &get_compute_family() const { return m_compute_family; }
        [[nodiscard]] const vk::Queue                  &get_transfer_queue() const { return m_transfer_queue; }
        [[nodiscard]] const vk::Queue                  &get_graphics_queue() const { return m_graphics_queue; }
        [[nodiscard]] const vk::Queue                  &get_present_queue() const { return m_present_queue; }
        [[nodiscard]] const vk::Queue                  &get_compute_queue() const { return m_compute_queue; }
        [[nodiscard]] const vk::SwapchainKHR           &get_swapchain() const { return m_swapchain; }
        [[nodiscard]] const SwapchainConfig            &get_swapchain_config() const { return m_swapchain_config; }
        [[nodiscard]] const std::vector<vk::Image>     &get_swapchain_images() const { return m_swapchain_images; }
        [[nodiscard]] const std::vector<vk::ImageView> &get_swapchain_image_views() const {
            return m_swapchain_image_views;
        }

        [[nodiscard]] uint32_t get_current_frame() const { return m_current_frame; }
        [[nodiscard]] uint32_t get_current_image_index() const { return m_current_image_index; }

        [[nodiscard]] const vk::Image &get_current_swapchain_image() const {
            return m_swapchain_images[m_current_image_index];
        }

        [[nodiscard]] const vk::ImageView &get_current_swapchain_image_view() const {
            return m_swapchain_image_views[m_current_image_index];
        }

        [[nodiscard]] const vk::CommandBuffer &get_current_command_buffer() const {
            return m_command_buffers[m_current_frame];
        }

        [[nodiscard]] const vk::Semaphore &get_current_image_available_semaphore() const {
            return m_image_available_semaphores[m_current_frame];
        }

        [[nodiscard]] const vk::Semaphore &get_current_render_finished_semaphore() const {
            return m_render_finished_semaphores[m_current_frame];
        }

        [[nodiscard]] const vk::Fence &get_current_in_flight_fence() const {
            return m_in_flight_fences[m_current_frame];
        }

        bool is_open() const;

        CommandRecorder begin_frame();

        void end_frame();

        vk::Semaphore create_semaphore() const;
        vk::Fence create_fence(bool signaled = false) const;

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

        vk::CommandPool                m_graphics_pool;
        std::vector<vk::CommandBuffer> m_command_buffers;

        uint32_t m_current_frame = 0;
        uint32_t m_current_image_index = 0;

        std::vector<vk::Semaphore> m_image_available_semaphores;
        std::vector<vk::Semaphore> m_render_finished_semaphores;
        std::vector<vk::Fence>     m_in_flight_fences;
    };
} // namespace kat
