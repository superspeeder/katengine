#include "kat/engine.hpp"


#include <set>
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
        create_command_buffers();
        create_syncs();
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

        queue_create_infos.reserve(unique_families.size());
        for (const auto &family : unique_families) {
            queue_create_infos.emplace_back(vk::DeviceQueueCreateFlags{}, family, 1, &queue_priority);
        }

        vk::PhysicalDeviceFeatures2 features2{};
        features2.features.geometryShader     = true;
        features2.features.tessellationShader = true;
        features2.features.largePoints        = true;
        features2.features.wideLines          = true;
        features2.features.fillModeNonSolid   = true;

        vk::PhysicalDeviceVulkan13Features v13f{};
        v13f.synchronization2 = true;
        v13f.dynamicRendering = true;
        v13f.inlineUniformBlock = true;

        features2.pNext = &v13f;

        m_device = m_physical_device.createDevice({ {}, queue_create_infos, {}, exts, {}, &features2 });

        m_graphics_queue = m_device.getQueue(m_graphics_family.value(), 0);
        m_transfer_queue = m_device.getQueue(m_transfer_family.value(), 0);
        m_present_queue  = m_device.getQueue(m_present_family.value(), 0);
        m_compute_queue  = m_device.getQueue(m_compute_family.value(), 0);
    }

    void Engine::create_swapchain() {
        vk::SwapchainCreateInfoKHR create_info{};

        const auto caps = m_physical_device.getSurfaceCapabilitiesKHR(m_surface);

        create_info.surface       = m_surface;
        create_info.minImageCount = caps.minImageCount + 1;
        if (caps.maxImageCount > 0 && create_info.minImageCount > caps.maxImageCount) {
            create_info.minImageCount = caps.maxImageCount;
        }

        m_swapchain_config.surface_format = select_surface_format();

        create_info.imageFormat     = m_swapchain_config.surface_format.format;
        create_info.imageColorSpace = m_swapchain_config.surface_format.colorSpace;

        if (caps.currentExtent.height == UINT32_MAX) {
            int w, h;
            glfwGetFramebufferSize(m_window, &w, &h);
            m_swapchain_config.extent = vk::Extent2D{
                std::clamp(static_cast<uint32_t>(w), caps.minImageExtent.width, caps.maxImageExtent.width),
                std::clamp(static_cast<uint32_t>(h), caps.minImageExtent.height, caps.maxImageExtent.height),
            };
        }
        else {
            m_swapchain_config.extent = caps.currentExtent;
        }

        create_info.imageExtent      = m_swapchain_config.extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;

        std::set<uint32_t>    families = { m_graphics_family.value(), m_present_family.value(),
                                           m_transfer_family.value() };
        std::vector<uint32_t> families_vec(families.cbegin(), families.cend());

        if (families_vec.size() == 1) {
            create_info.imageSharingMode = vk::SharingMode::eExclusive;
        }
        else {
            create_info.imageSharingMode = vk::SharingMode::eConcurrent;
            create_info.setQueueFamilyIndices(families_vec);
        }

        create_info.preTransform   = caps.currentTransform;
        create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

        m_swapchain_config.present_mode = select_present_mode();
        create_info.presentMode         = m_swapchain_config.present_mode;

        create_info.clipped      = true;
        create_info.oldSwapchain = m_swapchain;

        m_swapchain = m_device.createSwapchainKHR(create_info);

        if (create_info.oldSwapchain) {
            for (auto iv : m_swapchain_image_views)
                m_device.destroy(iv);

            m_device.destroy(create_info.oldSwapchain);
            // TODO: signal
        }

        m_swapchain_images = m_device.getSwapchainImagesKHR(m_swapchain);
        m_swapchain_image_views.clear();
        m_swapchain_image_views.reserve(m_swapchain_images.size());

        for (const auto &image : m_swapchain_images) {
            m_swapchain_image_views.push_back(m_device.createImageView(
                vk::ImageViewCreateInfo({}, image, vk::ImageViewType::e2D, m_swapchain_config.surface_format.format,
                                        STANDARD_COMPONENT_MAPPING, color_subresource_range(1, 1))));
        }

        // TODO: another signal
    }

    vk::SurfaceFormatKHR Engine::select_surface_format() const {
        const auto formats = m_physical_device.getSurfaceFormatsKHR(m_surface);

        for (const auto &format : formats) {
            if (format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear &&
                (format.format == vk::Format::eR8G8B8A8Srgb || format.format == vk::Format::eB8G8R8A8Srgb)) {
                return format;
            }
        }

        return formats[0];
    }

    vk::PresentModeKHR Engine::select_present_mode() const {
        for (const auto pms = m_physical_device.getSurfacePresentModesKHR(m_surface); const auto &pm : pms) {
            if (pm == vk::PresentModeKHR::eMailbox) {
                return pm;
            }
        }

        return vk::PresentModeKHR::eFifo;
    }

    void Engine::create_command_buffers() {
        m_graphics_pool = m_device.createCommandPool(
            vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_graphics_family.value()));

        m_command_buffers = m_device.allocateCommandBuffers(
            vk::CommandBufferAllocateInfo(m_graphics_pool, vk::CommandBufferLevel::ePrimary, MAX_FRAMES_IN_FLIGHT));
    }

    void Engine::create_syncs() {
        m_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_image_available_semaphores[i] = create_semaphore();
            m_render_finished_semaphores[i] = create_semaphore();
            m_in_flight_fences[i]           = create_fence(true);
        }
    }

    Engine::~Engine() {
        m_device.waitIdle();

        m_device.destroy(m_graphics_pool);

        for (const auto &o : m_render_finished_semaphores)
            m_device.destroy(o);

        for (const auto &o : m_in_flight_fences)
            m_device.destroy(o);

        for (const auto &o : m_image_available_semaphores)
            m_device.destroy(o);

        for (const auto &iv : m_swapchain_image_views)
            m_device.destroy(iv);

        m_device.destroy(m_swapchain);
        m_device.destroy();
        m_instance.destroy(m_surface);
        m_instance.destroy();
    }

    bool Engine::is_open() const {
        return !glfwWindowShouldClose(m_window);
    }

    CommandRecorder Engine::begin_frame() {
        [[maybe_unused]] auto ignored = m_device.waitForFences(get_current_in_flight_fence(), true, UINT64_MAX);
        m_device.resetFences(get_current_in_flight_fence());

        m_current_image_index =
            m_device.acquireNextImageKHR(m_swapchain, UINT64_MAX, get_current_image_available_semaphore(), {}).value;

        get_current_command_buffer().reset();
        get_current_command_buffer().begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        return CommandRecorder(get_current_command_buffer());
    }

    void Engine::end_frame() {
        get_current_command_buffer().end();

        auto cmd = get_current_command_buffer();
        auto ws  = get_current_image_available_semaphore();
        auto ss  = get_current_render_finished_semaphore();

        vk::PipelineStageFlags wdsm = vk::PipelineStageFlagBits::eTopOfPipe;

        vk::SubmitInfo submit_info{};
        submit_info.setCommandBuffers(cmd);
        submit_info.setWaitSemaphores(ws).setWaitDstStageMask(wdsm);
        submit_info.setSignalSemaphores(ss);

        m_graphics_queue.submit(submit_info, get_current_in_flight_fence());

        vk::PresentInfoKHR present_info{};
        present_info.setWaitSemaphores(ss).setSwapchains(m_swapchain).setImageIndices(m_current_image_index);
        [[maybe_unused]] auto ignored = m_present_queue.presentKHR(present_info);

        m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    vk::Semaphore Engine::create_semaphore() const {
        return m_device.createSemaphore(vk::SemaphoreCreateInfo());
    }

    vk::Fence Engine::create_fence(const bool signaled) const {
        return m_device.createFence(
            vk::FenceCreateInfo(signaled ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlags()));
    }
} // namespace kat
