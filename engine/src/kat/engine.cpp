#include "kat/engine.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace kat {
    GlobalState *globalState;

    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData) {

        auto mt = static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageType);

        std::string mts = vk::to_string(mt);

        switch (messageSeverity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                globalState->validationLogger->debug("{} {}", mts, pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                globalState->validationLogger->info("{} {}", mts, pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                globalState->validationLogger->warn("{} {}", mts, pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                globalState->validationLogger->error("{} {}", mts, pCallbackData->pMessage);
                break;
            default:
                break;
        }

        return false;
    }

    void setAppInfo(const std::string &name, const Version &version) {
        globalState->appName = name;
        globalState->appVersion = version;
    }

    void setValidationLayersEnabled(bool enabled) {
        globalState->enableValidationLayers = enabled;
    }

    void setApiDumpEnabled(bool enabled) { globalState->enableApiDump = enabled; }

    void init() {
        if (!globalState)
            globalState = new GlobalState();
    }

    void startup() {
        if (!globalState) {
            init();
        }

        if (globalState->startupComplete)
            return;

        globalState->startup();
    }

    void terminate() {
        if (globalState) {
            delete globalState;
            globalState = nullptr;
        }
    }

    std::shared_ptr<spdlog::logger> createEasyLogger(const std::string &name, GlobalState *globalState = nullptr) {
        if (!globalState) globalState = ::kat::globalState;

        std::string base_file = "logs/" + name + ".log";
        auto seperateFileLogger = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(base_file, SIZE_MAX, 30, true);

        std::array<std::shared_ptr<spdlog::sinks::sink>, 3> sinks = {seperateFileLogger, globalState->sharedFileSink, globalState->stdoutSink};
        return std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    }

    GlobalState::GlobalState() {
        sharedFileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/combined.log", SIZE_MAX, 30, true);
        stdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        mainLogger = createEasyLogger("main", this);
        validationLogger = createEasyLogger("validation", this);

        mainLogger->set_level(spdlog::level::debug);
        validationLogger->set_level(spdlog::level::debug);

        spdlog::set_default_logger(mainLogger);

        glfwInit();
        vk::defaultDispatchLoaderDynamic.init();
    }

    GlobalState::~GlobalState() {
        destroy(device);
        safeDestroy(debugMessenger);
        destroy(instance);
    }

    void GlobalState::startup() {
        spdlog::info("Starting up engine");

        vk::ApplicationInfo appInfo{};
        appInfo.setApiVersion(VK_API_VERSION_1_3)
                .setApplicationVersion(VK_MAKE_API_VERSION(appVersion.revision, appVersion.major, appVersion.minor, appVersion.patch))
                .setEngineVersion(VK_MAKE_API_VERSION(0, KATENGINE_VERSION_MAJOR, KATENGINE_VERSION_MINOR, KATENGINE_VERSION_PATCH))
                .setPApplicationName(appName.c_str())
                .setPEngineName("KatEngine");

        vk::InstanceCreateInfo instanceCreateInfo{};

        uint32_t count;
        const char **requiredExtensions = glfwGetRequiredInstanceExtensions(&count);

        std::vector<const char *> instanceExtensions;
        instanceExtensions.assign(requiredExtensions, requiredExtensions + count);

        std::vector<const char *> instanceLayers{};

        if (enableValidationLayers) {
            instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
        }

        if (enableApiDump) {
            instanceLayers.push_back("VK_LAYER_LUNARG_api_dump");
        }

        instanceCreateInfo.setPApplicationInfo(&appInfo)
                .setPEnabledExtensionNames(instanceExtensions)
                .setPEnabledLayerNames(instanceLayers);

        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoExt(
                {},
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
                vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding | vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
                debugCallback, nullptr);

        if (enableValidationLayers) {
            instanceCreateInfo.pNext = &debugUtilsMessengerCreateInfoExt;
        }

        instance = vk::createInstance(instanceCreateInfo);
        vk::defaultDispatchLoaderDynamic.init(instance);

        spdlog::debug("Created vulkan instance");

        if (enableValidationLayers) {
            debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoExt);
        }

        physicalDevice = instance.enumeratePhysicalDevices()[0];

        auto properties = physicalDevice.getProperties();

        spdlog::info("Using physical device: {}", properties.deviceName.data());
        {
            auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

            std::optional<uint32_t> optimalTransferExclusive;
            std::optional<uint32_t> optimalTransferNonExclusive;
            std::optional<uint32_t> transferExclusive;
            std::optional<uint32_t> transferNonExclusive;
            std::optional<uint32_t> mainf;

            uint32_t index = 0;
            for (const auto &qfp: queueFamilyProperties) {
                if (qfp.queueFlags & vk::QueueFlagBits::eGraphics && glfwGetPhysicalDevicePresentationSupport(instance, physicalDevice, index) && !mainf.has_value()) {
                    mainf = index;
                }

                if (qfp.queueFlags == (vk::QueueFlagBits::eTransfer | vk::QueueFlagBits::eSparseBinding)) {
                    optimalTransferExclusive = index;
                } else if (qfp.queueFlags & (vk::QueueFlagBits::eTransfer | vk::QueueFlagBits::eSparseBinding)) {
                    optimalTransferNonExclusive = index;
                } else if (qfp.queueFlags == vk::QueueFlagBits::eTransfer) {
                    transferExclusive = index;
                } else if (qfp.queueFlags & vk::QueueFlagBits::eTransfer) {
                    transferNonExclusive = index;
                }

                if (mainf.has_value() && optimalTransferExclusive.has_value()) {
                    break;
                }
            }

            if (!mainf.has_value()) {
                spdlog::critical("No main queue family.");
                throw std::runtime_error("No main queue family");
            }

            mainFamily = mainf.value();

            transferFamily = optimalTransferExclusive.value_or(optimalTransferNonExclusive.value_or(transferExclusive.value_or(transferNonExclusive.value_or(mainFamily))));
        }

        spdlog::debug("Main Queue Family: {}, Transfer Family: {}", mainFamily, transferFamily);

        bool queueFamiliesSame = mainFamily == transferFamily;

        std::vector<vk::DeviceQueueCreateInfo> dqcis;
        std::array<float, 1> queuePriorities =  {1.0f};
        dqcis.emplace_back(vk::DeviceQueueCreateFlags{}, mainFamily, queuePriorities);

        if (!queueFamiliesSame) {
            dqcis.emplace_back(vk::DeviceQueueCreateFlags{}, transferFamily, queuePriorities);
        }

        vk::PhysicalDeviceFeatures2 features2{};
        features2.features.fillModeNonSolid = true;
        features2.features.geometryShader = true;
        features2.features.tessellationShader = true;
        features2.features.wideLines = true;
        features2.features.largePoints = true;
        features2.features.multiDrawIndirect = true;
        features2.features.drawIndirectFirstInstance = true;
        features2.features.samplerAnisotropy = true;

        vk::PhysicalDeviceVulkan11Features v11f{};
        v11f.variablePointers = true;
        v11f.variablePointersStorageBuffer = true;
        v11f.shaderDrawParameters = true;

        vk::PhysicalDeviceVulkan12Features v12f{};
        v12f.bufferDeviceAddress = true;
        v12f.descriptorIndexing = true;
        v12f.timelineSemaphore = true;
        v12f.uniformBufferStandardLayout = true;

        vk::PhysicalDeviceVulkan13Features v13f{};
        v13f.dynamicRendering = true;
        v13f.synchronization2 = true;
        v13f.inlineUniformBlock = true;

        vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT eds3f{};
        eds3f.extendedDynamicState3PolygonMode = true;

        features2.pNext = &v11f;
        v11f.pNext = &v12f;
        v12f.pNext = &v13f;
        v13f.pNext = &eds3f;

        std::vector<const char*> extensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
        };

        device = physicalDevice.createDevice(vk::DeviceCreateInfo({}, dqcis, {}, extensions, nullptr, &features2));
        spdlog::info("Created logical device");

        vk::defaultDispatchLoaderDynamic.init(device);

        mainQueue = device.getQueue(mainFamily, 0);
        transferQueue = device.getQueue(transferFamily, 0);
    }

    void run() {
    }

    void eventloopCycle() {
    }

    void renderloopCycle() {
    }
}// namespace kat
