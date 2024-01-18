#include "kat/engine.hpp"

namespace kat
{
    Engine::Engine(const EngineConfig& engine_config)
    {

    }

    Engine::~Engine()
    {
        for (auto iv : m_swapchain_image_views) m_device.destroy(iv);

        m_device.destroy(m_swapchain);
        m_device.destroy();
        m_instance.destroy(m_surface);
        m_instance.destroy();
    }

    void Engine::create_window(const WindowConfig& config)
    {
        glfwInit();
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_window = glfwCreateWindow(config.size.x, config.size.y, config.title.c_str(), nullptr, nullptr);
    }

    void Engine::create_instance()
    {
        vk::ApplicationInfo application_info{};
        application_info.setApiVersion(VK_API_VERSION_1_3);

        uint32_t count;
        const char** exts = glfwGetRequiredInstanceExtensions(&count);

        m_instance = vk::createInstance(vk::InstanceCreateInfo({}, &application_info, 0, {}, count, exts));
    }

    void Engine::select_physical_device()
    {
        m_physical_device = m_instance.enumeratePhysicalDevices()[0];
    }

    void Engine::create_surface()
    {
        VkSurfaceKHR s;
        glfwCreateWindowSurface(m_instance, m_window, nullptr, &s);
        m_surface = s;
    }

    void Engine::select_queue_families()
    {
        std::optional<uint32_t> gt_family;
        std::optional<uint32_t> gtp_family;

        auto families = m_physical_device.getQueueFamilyProperties();
        uint32_t index = 0;
        for (const auto& f : families)
        {
            bool g = false;
            bool p = false;
            bool t = false;

            if (f.queueFlags & vk::QueueFlagBits::eGraphics)
            {
                g = true;
                m_graphics_family = index;
            }

            if (f.queueFlags & vk::QueueFlagBits::eTransfer)
            {
                t = true;
                m_transfer_family = index;
            }

            if (f.queueFlags & vk::QueueFlagBits::eCompute)
            {
                m_compute_family = index;
            }

            if (m_physical_device.getSurfaceSupportKHR(index, m_surface))
            {
                p = true;
                m_present_family = index;
            }

            if (g && t)
            {
                if (p)
                {
                    gtp_family = index;
                }

                gt_family = index;
            }

            if (m_graphics_family.has_value() && m_present_family.has_value() && m_transfer_family.has_value() && m_compute_family.has_value()) break;
        }


        m_has_gt_family = gt_family.has_value();
        m_has_gtpc_family = gtp_family.has_value();
        if (m_has_gtpc_family && *gtp_family == *m_compute_family) m_has_gtpc_family = true;

        if (m_has_gtp_family)
        {
            m_graphics_family = gtp_family.value();
            m_present_family = gtp_family.value();
            m_transfer_family = gtp_family.value();
        }
    }

    void Engine::create_device()
    {
    }

    void Engine::create_swapchain()
    {
    }

    void Engine::create_swapchain_image_views()
    {
    }
}
