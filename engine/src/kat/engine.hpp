#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <spdlog/spdlog.h>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <eventpp/callbacklist.h>

#include "kat/window.hpp"

#include "kat/vku.hpp"


#ifdef KATENGINE_DEBUG
#define KAT_DEBUG_SWITCH(dv, rv) dv
#else
#define KAT_DEBUG_SWITCH(dv, rv) rv
#endif

#ifndef KATENGINE_UNCHECKED_DESTROY
#define KATENGINE_UNCHECKED_DESTROY KAT_DEBUG_SWITCH(0, 1)
#endif

#define KAT_IS_DEBUG KAT_DEBUG_SWITCH(true, false)

#if KATENGINE_UNCHECKED_DESTROY
#define KAT_DESTROY_SAFE_CHECK(o) if (o)
#else
#define KAT_DESTROY_SAFE_CHECK(o)
#endif

namespace kat {
    class Window;

    struct Version {
        int major, minor, patch, revision = 0;
    };

    struct GlobalState {
        std::string appName = "Application";
        Version appVersion = Version{0, 1, 0};

        bool enableApiDump = false;
        bool enableValidationLayers = false;

        bool startupComplete = false;

        spdlog::level::level_enum defaultLogLevel = KAT_DEBUG_SWITCH(spdlog::level::debug, spdlog::level::info);

        std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> stdoutSink;
        std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> sharedFileSink;
        std::shared_ptr<spdlog::logger> mainLogger;
        std::shared_ptr<spdlog::logger> validationLogger;

        std::atomic<uint32_t> activeWindowCount = 0;
        std::unordered_map<size_t, std::shared_ptr<Window>> activeWindows;
        std::atomic<size_t> nextWindowId;

        bool seperateRenderAndUpdateThreads = false;

        vk::Instance instance;
        vk::DebugUtilsMessengerEXT debugMessenger;
        vk::PhysicalDevice physicalDevice;
        vk::Device device;

        uint32_t mainFamily;
        uint32_t transferFamily;

        vk::Queue mainQueue;
        vk::Queue transferQueue;

        vk::CommandPool mainPool;
        vk::CommandPool transferPool;

        vk::CommandPool otcPool;

        std::mutex mutOTCPool;
        std::mutex mutOTCL;

        // the shared ptr can be used for lifetime preservation.
        std::queue<std::tuple<vk::Fence, bool, vk::CommandBuffer, std::shared_ptr<void>>> otcl; // first fence is waited on, if second is false then the fence isn't destroyed (assume that the fence is used elsewhere);

        //        std::jthread otclCleaner;

        bool doRenderSetup = false;
        bool isRenderSetupOnlyOperation = false; // will make render setup also signal render completion. largely for use while I'm developing stuff and don't have any rendering code yet (so the app actually updates).

        std::atomic_bool otclcStop = false;

        friend void init();
        friend void startup();
        friend void terminate();

        void wrapup();

      private:
        GlobalState();

      public:
        ~GlobalState();

      private:
        void startup();
    };

    extern GlobalState *globalState;

    void setAppInfo(const std::string &name, const Version &version);

    void setValidationLayersEnabled(bool enabled);
    void setApiDumpEnabled(bool enabled);

    void init();
    void startup();

    void terminate();

    template<typename T>
    concept instance_destructible = requires(const vk::Instance &instance, const T &object) {
        instance.destroy(object);
    };

    template<typename T>
    concept device_destructible = requires(const vk::Device &device, const T &object) {
        device.destroy(object);
    } && std::convertible_to<T, bool>;

    template<typename T>
    concept device_freeable = requires(const vk::Device &device, const T &object) {
        device.free(object);
    } && std::convertible_to<T, bool>;

    inline void destroy(const vk::Instance &instance) {
        KAT_DESTROY_SAFE_CHECK(instance)
        instance.destroy();
    }

    inline void destroy(const vk::Device &device) {
        KAT_DESTROY_SAFE_CHECK(device)
        device.destroy();
    }

    template<instance_destructible T>
    inline void destroy(const T &object) {
        KAT_DESTROY_SAFE_CHECK(object)
        globalState->instance.destroy(object);
    }

    template<device_destructible T>
    inline void destroy(const T &object) {
        KAT_DESTROY_SAFE_CHECK(object)
        globalState->device.destroy(object);
    }

    inline void safeDestroy(const vk::Instance &instance) {
        if (instance) instance.destroy();
    }

    inline void safeDestroy(const vk::Device &device) {
        if (device) device.destroy();
    }

    template<instance_destructible T>
    inline void safeDestroy(const T &object) {
        if (object) globalState->instance.destroy(object);
    }

    template<device_destructible T>
    inline void safeDestroy(const T &object) {
        if (object) globalState->device.destroy(object);
    }

    void run();

    void eventloopCycle();
    void renderloopCycle();


    template<typename T>
    concept sccompatible = ((std::same_as<std::remove_cvref_t<decltype(T::sType)>, vk::StructureType> ||
                             std::same_as<std::remove_cvref_t<decltype(T::sType)>, VkStructureType>) &&
                            (std::same_as<std::remove_cvref_t<decltype(T::pNext)>, void *> ||
                             std::same_as<std::remove_cvref_t<decltype(T::pNext)>, const void *>) &&
                            offsetof(T, pNext) - offsetof(T, sType) == offsetof(vk::BaseOutStructure, pNext)) ||
                           std::same_as<T, vk::BaseOutStructure> || std::same_as<T, vk::BaseInStructure>; // i.e. are they in the right place

    inline void *offsetptr(void *a, size_t o) {
        return static_cast<char *>(a) + o;
    };

    template<sccompatible T>
    inline vk::BaseOutStructure *baseOutStructure(T *v) {
        return static_cast<vk::BaseOutStructure *>(offsetptr(v, offsetof(T, sType)));
    };

    template<sccompatible T>
    inline vk::BaseOutStructure *endofchain(T *top) {
        if (top == nullptr) {
            return nullptr;
        }
        vk::BaseOutStructure *b = baseOutStructure(top);
        while (b->pNext) {
            b = static_cast<vk::BaseOutStructure *>(b->pNext);
        }

        return b;
    };

    class DynamicStructureChain {
        vk::BaseOutStructure *tail = nullptr;
        vk::BaseOutStructure *head = nullptr;

      public:
        inline DynamicStructureChain() = default;

        template<sccompatible T>
        inline void push(T *structure) {
            if (structure == nullptr) {
                return;
            }

            auto *bos = baseOutStructure(structure);

            if (head == nullptr) {
                head = bos;
            } else {
                tail->pNext = bos;
            }

            auto *end = endofchain(bos);
            tail = end;
        };

        [[nodiscard]] inline vk::BaseOutStructure *get() const {
            return head;
        };

        DynamicStructureChain(const DynamicStructureChain &) = delete;
        DynamicStructureChain &operator=(const DynamicStructureChain &) = delete;
    };

    namespace vku {
        vk::Semaphore createSemaphore();
        vk::Fence createFence();
        vk::Fence createFenceSignaled();
        vk::Event createEvent();
        vk::Event createDeviceOnlyEvent();

        void waitFence(const vk::Fence &fence);
        void resetFence(vk::Fence fence);

        [[nodiscard]] bool getEventStatus(const vk::Event& event);
        void setEvent(const vk::Event& event);
        void resetEvent(const vk::Event& event);

        struct OTCSync {
            vk::Semaphore signal, wait;
            vk::PipelineStageFlags2 waitStage = vk::PipelineStageFlagBits2::eTopOfPipe;
            vk::PipelineStageFlags2 signalStage = vk::PipelineStageFlagBits2::eBottomOfPipe;
        };

        void otc(const std::function<void(const vk::CommandBuffer &)> &f, OTCSync sync = {}, const std::shared_ptr<void> &ptr = {});
        void otc(const std::function<void(const vk::CommandBuffer &)> &f, vk::Fence fence, OTCSync sync = {}, const std::shared_ptr<void> &ptr = {});
    } // namespace vku
} // namespace kat
