#include "kat/engine.hpp"


#include <unordered_set>

namespace kat {
    Engine::Engine(const EngineConfig &engine_config) {
        create_window(engine_config.window_config);
        create_instance();
        create_surface();
        select_physical_device();
        select_queue_families();
        create_device();
        create_swapchain();
        create_swapchain_image_views();
    }

    Engine::~Engine() {
        for (auto iv : m_swapchain_image_views)
            m_device.destroy(iv);

        m_device.destroy(m_swapchain);
        m_device.destroy();
        m_instance.destroy(m_surface);
        m_instance.destroy();
    }

    void Engine::create_window(const WindowConfig &config) {
        glfwInit();
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_window = glfwCreateWindow(config.size.x, config.size.y, config.title.c_str(), nullptr, nullptr);
    }

    void Engine::create_instance() {
        vk::ApplicationInfo application_info{};
        application_info.setApiVersion(VK_API_VERSION_1_3);

        uint32_t     count;
        const char **exts = glfwGetRequiredInstanceExtensions(&count);

        m_instance = vk::createInstance(vk::InstanceCreateInfo({}, &application_info, 0, {}, count, exts));
    }

    void Engine::select_physical_device() {
        m_physical_device = m_instance.enumeratePhysicalDevices()[0];
    }

    void Engine::create_surface() {
        VkSurfaceKHR s;
        glfwCreateWindowSurface(m_instance, m_window, nullptr, &s);
        m_surface = s;
    }

    void Engine::select_queue_families() {
        std::optional<uint32_t> gt_family;
        std::optional<uint32_t> gtp_family;

        auto     families = m_physical_device.getQueueFamilyProperties();
        uint32_t index    = 0;
        for (const auto &f : families) {
            bool g = false;
            bool p = false;
            bool t = false;

            if (f.queueFlags & vk::QueueFlagBits::eGraphics) {
                g                 = true;
                m_graphics_family = index;
            }

            if (f.queueFlags & vk::QueueFlagBits::eTransfer) {
                t                 = true;
                m_transfer_family = index;
            }

            if (f.queueFlags & vk::QueueFlagBits::eCompute) {
                m_compute_family = index;
            }

            if (m_physical_device.getSurfaceSupportKHR(index, m_surface)) {
                p                = true;
                m_present_family = index;
            }

            if (g && t) {
                if (p) {
                    gtp_family = index;
                }

                gt_family = index;
            }

            if (m_graphics_family.has_value() && m_present_family.has_value() && m_transfer_family.has_value() &&
                m_compute_family.has_value())
                break;
        }

        m_has_gt_family   = gt_family.has_value();
        m_has_gtpc_family = gtp_family.has_value();
        if (m_has_gtpc_family && *gtp_family == *m_compute_family)
            m_has_gtpc_family = true;

        if (m_has_gtp_family) {
            m_graphics_family = gtp_family.value();
            m_present_family  = gtp_family.value();
            m_transfer_family = gtp_family.value();
        }
    }

    void Engine::create_device() {
        std::vector<const char *> exts = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
        const std::unordered_set<uint32_t>     unique_families = { m_graphics_family.value(), m_present_family.value(),
                                                                   m_transfer_family.value(), m_compute_family.value() };
        float                                  queue_priority  = 1.0f;

        for (const auto &family : unique_families) {
            queue_create_infos.emplace_back(vk::DeviceQueueCreateFlags{}, family, 1, &queue_priority);
        }

        vk::PhysicalDeviceFeatures2 features2{};
        features2.features.geometryShader     = true;
        features2.features.tessellationShader = true;
        features2.features.largePoints        = true;
        features2.features.wideLines          = true;
        features2.features.fillModeNonSolid   = true;

        auto create_info = vk::DeviceCreateInfo({}, queue_create_infos, {}, exts, {}, &features2);

        m_device = m_physical_device.createDevice(create_info);

        m_graphics_queue = m_device.getQueue(m_graphics_family.value(), 0);
        m_transfer_queue = m_device.getQueue(m_transfer_family.value(), 0);
        m_present_queue  = m_device.getQueue(m_present_family.value(), 0);
        m_compute_queue  = m_device.getQueue(m_compute_family.value(), 0);
    }

    void Engine::create_swapchain() {
        vk::SwapchainCreateInfoKHR create_info{};

        auto caps = m_physical_device.getSurfaceCapabilitiesKHR(m_surface);
        auto formats = m_physical_device.getSurfaceFormatsKHR(m_surface);
        auto present_modes = m_physical_device.getSurfacePresentModesKHR(m_surface);

        create_info.surface = m_surface;
        create_info.minImageCount = caps.minImageCount + 1;
        if (caps.maxImageCount > 0 && create_info.minImageCount > caps.maxImageCount) {
            create_info.minImageCount = caps.maxImageCount;
        }

        create_info.imageFormat
    }

    void Engine::create_swapchain_image_views() {}
} // namespace kat
