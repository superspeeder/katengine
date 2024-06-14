#pragma once

#include <vulkan/vulkan.hpp>

namespace kat::vkw {
    extern vk::Optional<const vk::AllocationCallbacks> allocator;

    struct ContextSettings final {

    };

    class Context final {
      public:
        Context(const ContextSettings& settings);
        ~Context();

        [[nodiscard]] inline const vk::Instance& instance() const noexcept { return m_Instance; }
        [[nodiscard]] inline const vk::PhysicalDevice& gpu() const noexcept { return m_Gpu; };
        [[nodiscard]] inline const vk::Device& device() const noexcept { return m_Device; };

        [[nodiscard]] inline const uint32_t& mainQueueFamily() const noexcept { return m_MainQueueFamily; };
        [[nodiscard]] inline const vk::Queue& mainQueue() const noexcept { return m_MainQueue; };


      private:

        vk::Instance m_Instance;
        vk::PhysicalDevice m_Gpu;
        vk::Device m_Device;

        uint32_t m_MainQueueFamily;
        // TODO: Exclusive transfer family;

        vk::Queue m_MainQueue;

        vk::DebugUtilsMessengerEXT m_DebugMessenger;
    };


    extern Context* context;

    void initContext(const ContextSettings& settings);
    void destroyContext();

}