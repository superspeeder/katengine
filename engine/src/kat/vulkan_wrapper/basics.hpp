#pragma once

#include <vulkan/vulkan.hpp>

#include <cstdarg>

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

    template<typename T>
    concept instance_destructible = requires(vk::Instance instance, T object) { instance.destroy(object); };

    template<typename T>
    concept device_destructible = requires(vk::Device device, T object) { device.destroy(object); };

    template<typename T>
    concept easy_destructible = instance_destructible<T> || device_destructible<T>;

    template<easy_destructible T, easy_destructible... Ts>
    inline void destroy_(const T& object, const Ts&... objects) {
        if constexpr (instance_destructible<T>) context->instance().destroy(object);
        if constexpr (device_destructible<T>) context->device().destroy(object);
        if constexpr (std::tuple_size_v<std::tuple<Ts...>> != 0) destroy_(objects...);
    };

    template<easy_destructible... Args>
    inline void destroy(const Args&... args) {
        destroy_(args...);
    };

    inline void waitFence(const vk::Fence &fence) {
        context->device().waitForFences(fence, true, UINT64_MAX);
    };

    inline void resetFence(const vk::Fence &fence) {
        context->device().resetFences(fence);
    };

    inline vk::Fence createFence() {
        const static vk::FenceCreateInfo fci{};
        return context->device().createFence(fci);
    };

    inline vk::Fence createSignaledFence() {
        const static vk::FenceCreateInfo fci{vk::FenceCreateFlagBits::eSignaled};
        return context->device().createFence(fci);
    };

    inline vk::Semaphore createSemaphore() {
        const static vk::SemaphoreCreateInfo sci{};
        return context->device().createSemaphore(sci);
    };
}