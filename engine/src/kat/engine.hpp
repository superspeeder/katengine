#pragma once



#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <set>
#include <unordered_set>

#include <string>
#include <vector>

#include <memory>

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

        std::unordered_set<GLFWwindow *> activeWindows;

        vk::Instance instance;
        vk::DebugUtilsMessengerEXT debugMessenger;
        vk::PhysicalDevice physicalDevice;
        vk::Device device;

        uint32_t mainFamily;
        uint32_t transferFamily;

        vk::Queue mainQueue;
        vk::Queue transferQueue;

        friend void init();
        friend void startup();
        friend void terminate();

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


}// namespace kat
