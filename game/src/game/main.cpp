#include <kat/core.hpp>
#include <kat/os/window.hpp>
#include <kat/vulkan_wrapper/basics.hpp>
#include <kat/vulkan_wrapper/swapchain.hpp>

#include <glm/gtx/color_space.hpp>

#include <memory>

void runApplication() {
    kat::os::WindowConfiguration windowConfig{};
    windowConfig.title = "Hello!";
    windowConfig.mode.mode = kat::os::WindowModeType::WINDOWED;
    windowConfig.mode.settings = kat::os::WindowedMode{.size = {3840, 2160}, .resizable = false};
//    windowConfig.mode.mode = kat::os::WindowModeType::EXCLUSIVE_FULLSCREEN;
//    windowConfig.mode.settings = kat::os::FullscreenMode{.monitorId = 1ull};

    std::shared_ptr<kat::os::Window> window = std::make_shared<kat::os::Window>(windowConfig);

    kat::vkw::ContextSettings contextSettings{};
    kat::vkw::initContext(contextSettings);

    spdlog::info("GPU: {}", kat::vkw::context->gpu().getProperties().deviceName.data());

    auto easySwapchain = std::make_shared<kat::vkw::EasySwapchain>(window);

    float h = 0.0f;

    double thisFrame = glfwGetTime();
    double deltaTime = 1.0f / 60.f;
    double lastFrame = thisFrame - deltaTime;

    std::function<void()> onRender = [&]() {
        easySwapchain->renderAndPresent([&](const vk::CommandBuffer &cmd, const kat::vkw::CurrentFrameInfo &cfi) {
            vk::ImageMemoryBarrier2 imb1{};
            imb1.srcAccessMask = vk::AccessFlagBits2::eNone;
            imb1.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
            imb1.oldLayout = vk::ImageLayout::eUndefined;
            imb1.newLayout = vk::ImageLayout::eTransferDstOptimal;
            imb1.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
            imb1.dstStageMask = vk::PipelineStageFlagBits2::eClear;
            imb1.image = cfi.image;
            imb1.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);


            vk::ImageMemoryBarrier2 imb2{};
            imb2.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
            imb2.dstAccessMask = vk::AccessFlagBits2::eNone;
            imb2.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            imb2.newLayout = vk::ImageLayout::ePresentSrcKHR;
            imb2.srcStageMask = vk::PipelineStageFlagBits2::eClear;
            imb2.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
            imb2.image = cfi.image;
            imb2.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

            h = fmodf(h + deltaTime * 60.0f, 360.0f);
            glm::vec3 color = glm::rgbColor(glm::vec3(h, 1.0f, 1.0f));

            vk::ClearColorValue clearColor = vk::ClearColorValue(color.r, color.g, color.b, 1.0f);
            std::array<vk::ImageSubresourceRange, 1> subresourceRanges = {vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)};

            cmd.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, imb1));
            cmd.clearColorImage(cfi.image, vk::ImageLayout::eTransferDstOptimal, clearColor, subresourceRanges);
            cmd.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, imb2));
        });
        lastFrame = thisFrame;
        thisFrame = glfwGetTime();
        deltaTime = thisFrame - lastFrame;
    };

    window->setRenderFunction(onRender);

    while (!window->shouldClose()) {
        kat::os::Window::pollEvents();
        onRender();
    }

    kat::vkw::context->device().waitIdle();
}

KAT_ENTRYPOINT(::runApplication)
