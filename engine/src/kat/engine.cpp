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

    std::shared_ptr<spdlog::logger> createEasyLogger(const std::string &name, GlobalState *gs = nullptr) {
        if (!gs) gs = globalState;

        std::string base_file = "logs/" + name + ".log";
        auto seperateFileLogger = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(base_file, SIZE_MAX, 30, true);

        std::array<std::shared_ptr<spdlog::sinks::sink>, 3> sinks = {seperateFileLogger, gs->sharedFileSink, gs->stdoutSink};
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
        device.waitIdle();

        otclcStop = true;
        otclCleaner.join();

        destroy(transferPool);
        destroy(mainPool);

        destroy(device);
        safeDestroy(debugMessenger);
        destroy(instance);
    }

    bool otclcCheckStop() {
        return !globalState->otclcStop;
    }

    void otclc() {
        vk::Fence wait;
        bool managed;
        vk::CommandBuffer cmdb;

        while (otclcCheckStop())
        {
            std::this_thread::yield();
            {
                std::lock_guard guard(globalState->mutOTCL);
                if (globalState->otcl.empty()) continue;
                std::tie(wait, managed, cmdb) = globalState->otcl.front();
                globalState->otcl.pop();
            }

            if (globalState->device.getFenceStatus(wait) == vk::Result::eNotReady) {
                std::lock_guard guard(globalState->mutOTCL);
                globalState->otcl.emplace(wait, managed, cmdb);
            } else {
                spdlog::debug("Freed command buffer: {:x}", reinterpret_cast<uint64_t>(static_cast<VkCommandBuffer>(cmdb)));
                {
                    std::lock_guard lk(globalState->mutMainPool);
                    globalState->device.freeCommandBuffers(globalState->mainPool, cmdb);
                }
                if (managed) {
                    destroy(wait);
                }
            }
        }
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
        std::array<float, 1> queuePriorities = {1.0f};
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

        std::vector<const char *> extensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
        };

        device = physicalDevice.createDevice(vk::DeviceCreateInfo({}, dqcis, {}, extensions, nullptr, &features2));
        spdlog::info("Created logical device");

        vk::defaultDispatchLoaderDynamic.init(device);

        mainQueue = device.getQueue(mainFamily, 0);
        transferQueue = device.getQueue(transferFamily, 0);

        mainPool = device.createCommandPool(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mainFamily));
        transferPool = device.createCommandPool(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, transferFamily));

        otclCleaner = std::jthread(otclc);
    }


    void run() {
        while (globalState->activeWindowCount > 0) {
            eventloopCycle();
            renderloopCycle();
        }
    }

    void eventloopCycle() {
        glfwPollEvents();
    }

    struct PI_ {
        vk::SwapchainKHR swapchain;
        uint32_t imageIndex;
        vk::Semaphore sem;
    };

    std::vector<PI_> pinfos;

    void doWindowRender(const std::unique_ptr<Window> &second) {
        if (second->acquireFrame()) {
            const auto* sync = second->getCurrentFrameResources().sync;

            if (globalState->doRenderSetup) {
                vku::OTCSync otcs{};
                otcs.wait = sync->imageAvailableSemaphore; // todo: make and pass a semaphore to the rendering code for signal when the next option isn't available (to replace imageAvailableSemaphore)
                if (globalState->isRenderSetupOnlyOperation) {
                    otcs.signal = sync->renderFinishedSemaphore;
                }

                vku::otc([&](const vk::CommandBuffer& cmd) {
                    vk::ImageMemoryBarrier2 imb{};
                    imb.image = second->getCurrentFrameResources().image;
                    imb.srcAccessMask = vk::AccessFlagBits2::eNone;
                    imb.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
                    imb.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
                    imb.dstStageMask = vk::PipelineStageFlagBits2::eClear;
                    imb.srcQueueFamilyIndex = globalState->mainFamily;
                    imb.dstQueueFamilyIndex = globalState->mainFamily;
                    imb.oldLayout = vk::ImageLayout::eUndefined;
                    imb.newLayout = vk::ImageLayout::eTransferDstOptimal;
                    imb.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

                    float n = (sinf(float(glfwGetTime())) + 1.0f) / 2.0f;

                    vk::ClearColorValue clearValue{n, 0.0f, 0.0f, 1.0f};
                    vk::ImageSubresourceRange range{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

                    vk::ImageMemoryBarrier2 imb2{};
                    imb2.image = second->getCurrentFrameResources().image;
                    imb2.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
                    imb2.dstAccessMask = vk::AccessFlagBits2::eNone;
                    imb2.srcStageMask = vk::PipelineStageFlagBits2::eClear;
                    imb2.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
                    imb2.srcQueueFamilyIndex = globalState->mainFamily;
                    imb2.dstQueueFamilyIndex = globalState->mainFamily;
                    imb2.oldLayout = vk::ImageLayout::eTransferDstOptimal;
                    imb2.newLayout = vk::ImageLayout::ePresentSrcKHR;
                    imb2.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

                    cmd.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, imb));
                    cmd.clearColorImage(second->getCurrentFrameResources().image, vk::ImageLayout::eTransferDstOptimal, clearValue, range);
                    cmd.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, imb2));
                }, sync->inFlightFence, otcs);
            }
            // TODO: render frame. for now we just do some simple ops

            pinfos.push_back(PI_{.swapchain = second->getSwapchain(), .imageIndex = second->getCurrentFrameResources().imageIndex, .sem = sync->renderFinishedSemaphore});
        }
    }

    void renderloopCycle() {
        pinfos.clear();

        for (const auto &window: globalState->activeWindows) {
            doWindowRender(window.second);
        }

        std::vector<vk::SwapchainKHR> storage0;
        std::vector<uint32_t> storage1;
        std::vector<vk::Semaphore> storage2;

        for (const auto pi : pinfos) {
            storage0.push_back(pi.swapchain);
            storage1.push_back(pi.imageIndex);
            storage2.push_back(pi.sem);
        }

        if (pinfos.empty()) return;

        vk::PresentInfoKHR present{};
        present.setSwapchains(storage0);
        present.setImageIndices(storage1);
        present.setWaitSemaphores(storage2);

        [[maybe_unused]] auto _ = globalState->mainQueue.presentKHR(present);
    }

    namespace vku {
        vk::Semaphore createSemaphore() {
            static const vk::SemaphoreCreateInfo sci{};
            return globalState->device.createSemaphore(sci);
        }

        vk::Fence createFence() {
            static const vk::FenceCreateInfo fci{};
            return globalState->device.createFence(fci);
        }

        vk::Fence createFenceSignaled() {
            static const vk::FenceCreateInfo fci(vk::FenceCreateFlagBits::eSignaled);
            return globalState->device.createFence(fci);
        }

        void waitFence(const vk::Fence &fence) {
            auto _ = globalState->device.waitForFences(fence, true, UINT64_MAX);
        }

        void resetFence(vk::Fence fence) {
            globalState->device.resetFences(fence);
        }

        void otc(const std::function<void(const vk::CommandBuffer &)> &f, OTCSync sync) {
            vk::CommandBuffer cmdb;
            {
                std::lock_guard lk(globalState->mutMainPool);
                cmdb= globalState->device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(globalState->mainPool, vk::CommandBufferLevel::ePrimary, 1))[0];
            }
            cmdb.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
            f(cmdb);
            cmdb.end();

            vk::SubmitInfo si{};
            si.setCommandBuffers(cmdb);
            si.setWaitSemaphores(sync.wait);
            vk::PipelineStageFlags stage = vk::PipelineStageFlagBits::eTopOfPipe;

            si.setWaitDstStageMask(stage);
            si.setSignalSemaphores(sync.signal);

            vk::Fence fence = createFence();

            globalState->mainQueue.submit(si, fence);

            {
                std::lock_guard lock(globalState->mutOTCL);
                globalState->otcl.emplace(fence, true, cmdb);
            }
        }

        void otc(const std::function<void(const vk::CommandBuffer &)> &f, vk::Fence fence, OTCSync sync) {
            vk::CommandBuffer cmdb;
            {
                std::lock_guard lk(globalState->mutMainPool);
                cmdb= globalState->device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(globalState->mainPool, vk::CommandBufferLevel::ePrimary, 1))[0];
            }
            cmdb.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
            f(cmdb);
            cmdb.end();

            vk::SubmitInfo si{};
            si.setCommandBuffers(cmdb);
            si.setWaitSemaphores(sync.wait);
            vk::PipelineStageFlags stage = vk::PipelineStageFlagBits::eTopOfPipe;

            si.setWaitDstStageMask(stage);
            si.setSignalSemaphores(sync.signal);

            globalState->mainQueue.submit(si, fence);

            {
                std::lock_guard lock(globalState->mutOTCL);
                globalState->otcl.emplace(fence, false, cmdb);
            }
        }
    } // namespace vku
} // namespace kat
