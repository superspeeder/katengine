#include "basics.hpp"

#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#include <array>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace kat::vkw {
    vk::Optional<const vk::AllocationCallbacks> allocator = nullptr;
    Context* context = nullptr;

    void initContext(const ContextSettings &settings) {
        if (context) {
            spdlog::error("Context already initialized");
            return;
        }

        context = new Context(settings);
    }

    // no messages for context being nullptr because this gets called always during kat::terminateSubsystems(), which doesn't care if the global vulkan context exists or not.
    void destroyContext() {
        if (context) {
            delete context;
            context = nullptr;
        }
    }

    Context::Context(const ContextSettings &settings) {
        {
            VULKAN_HPP_DEFAULT_DISPATCHER.init();

            vk::ApplicationInfo applicationInfo{};
            applicationInfo.setApiVersion(vk::ApiVersion13);
            applicationInfo.setEngineVersion(vk::makeApiVersion(0, KATENGINE_VERSION_MAJOR, KATENGINE_VERSION_MINOR, KATENGINE_VERSION_PATCH));
            applicationInfo.setPEngineName("KatEngine");
            applicationInfo.setPApplicationName("Unknown");
            applicationInfo.setApplicationVersion(vk::makeApiVersion(0, 1, 0, 0));

            uint32_t ecount;
            const char** es = glfwGetRequiredInstanceExtensions(&ecount);

            std::vector<const char*> layers;
            std::vector<const char*> extensions(es, es + ecount);

            // TODO: validation layers (ill let the api dump one be activated through overrides, seems more sensible to me)

            m_Instance = vk::createInstance(vk::InstanceCreateInfo({}, &applicationInfo, layers, extensions), allocator);
            VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Instance);
        }

        {
            // TODO: smarter gpu selection
            auto pds = m_Instance.enumeratePhysicalDevices();
            if (pds.empty()) throw std::runtime_error("No supported GPU.");
            m_Gpu = pds[0];
        }

        {
            auto queueFamilyProperties = m_Gpu.getQueueFamilyProperties();
            m_MainQueueFamily = UINT32_MAX;
            uint32_t index = 0;

            static constexpr auto MAIN_FLAGS = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute;

            for (const auto& p : queueFamilyProperties) {
                if ((p.queueFlags & MAIN_FLAGS) == MAIN_FLAGS && glfwGetPhysicalDevicePresentationSupport(m_Instance, m_Gpu, index)) {
                    m_MainQueueFamily = index;
                    break; // change this when we get to exclusive transfer family
                }

                index++;
            }

            if (m_MainQueueFamily == UINT32_MAX) throw std::runtime_error("No valid main queue family");
        }

        {
            std::array<float, 1> queuePriorities = {1.0f};

            std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};

            queueCreateInfos.emplace_back(vk::DeviceQueueCreateFlags(), m_MainQueueFamily, queuePriorities);

            std::vector<const char*> extensions = {
                    VK_KHR_SWAPCHAIN_EXTENSION_NAME
            };

            vk::PhysicalDeviceFeatures2 features{};
            // include features as needed

            m_Device = m_Gpu.createDevice(vk::DeviceCreateInfo({}, queueCreateInfos, {}, extensions, nullptr, &features), allocator);
            VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Device);
        }
    }

    Context::~Context() {
        m_Device.destroy();
        m_Instance.destroy();
    }
} // namespace kat::vkw