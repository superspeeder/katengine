#include "game/main.hpp"

#include <cstdlib>
#include <iostream>

int main(int argc, char *argv[]) {
    kat::init();

    kat::setValidationLayersEnabled(true);
    //    kat::setApiDumpEnabled(true);

    kat::startup();

    kat::globalState->doRenderSetup = true;
    kat::globalState->isRenderSetupOnlyOperation = true;

    kat::Window *window;
    size_t windowId;
    std::tie(windowId, window) = kat::Window::create("Hello!", vk::Extent2D{800, 600}, kat::WindowOptions{false});

    window->setWindowHandler(std::make_shared<WindowHandler>(window));

    kat::run();

    kat::terminate();
    return EXIT_SUCCESS;
}

WindowHandler::WindowHandler(kat::Window *window) : m_Window(window), kat::BaseWindowHandler() {
    m_RenderPass = std::make_shared<kat::RenderPass>(kat::RenderPassInfo(
            {
                    kat::AttachmentInfo{window->getSurfaceFormat().format, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR, kat::LSO_STANDARD_CLEAR_STORE, kat::LSO_DONT_CARE, vk::SampleCountFlagBits::e1},
            },
            {
                    kat::SubpassInfo{vk::PipelineBindPoint::eGraphics, 0, {}, {kat::AttachmentReference{0, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageAspectFlagBits::eColor}}, {}, std::nullopt, {}, std::nullopt, std::nullopt, std::nullopt, std::nullopt},
            },
            {kat::SubpassDependency{kat::SubpassReference{vk::SubpassExternal, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlags()}, {0, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite}}}));

    for (const auto& iv : window->getImageViews()) {
        m_Framebuffers.push_back(kat::globalState->device.createFramebuffer(vk::FramebufferCreateInfo({}, m_RenderPass->get(), iv, m_Window->getCurrentExtent().width, m_Window->getCurrentExtent().height, 1)));
    }

    thisFrame = glfwGetTime();
    delta = 1.0f / 0.6f; // due to the 1:100 frame to delta ratio im using for smoothing purposes rn.
    lastFrame = thisFrame - delta;
}

void WindowHandler::onRender(const std::shared_ptr<kat::Window> &window, const kat::WindowFrameResources &resources) {
    kat::vku::OTCSync otcs{};
    otcs.wait = resources.sync->imageAvailableSemaphore;
    otcs.signal = resources.sync->renderFinishedSemaphore;

    kat::vku::otc([&](const vk::CommandBuffer &cmd) {
        float n = (sinf(float(glfwGetTime())) + 1.0f) / 2.0f;

        vk::ClearColorValue clearValue{n, 0.0f, 0.0f, 1.0f};
        std::vector<vk::ClearValue> cvs = {vk::ClearValue(clearValue)};

        cmd.beginRenderPass2(vk::RenderPassBeginInfo(m_RenderPass->get(), m_Framebuffers[resources.imageIndex], vk::Rect2D(vk::Offset2D(0, 0), window->getCurrentExtent()), cvs), vk::SubpassBeginInfo());

        cmd.endRenderPass2(vk::SubpassEndInfo());

    }, resources.sync->inFlightFence, otcs, window);

    fcounter++;

    if (fcounter % 100 == 0) {
        // pretend frames are actually 100 frames long
        lastFrame = thisFrame;
        thisFrame = glfwGetTime();
        delta = thisFrame - lastFrame;
        double fps = 100.0f / delta;
        highest_fps = std::max(highest_fps, fps);
    }
}

WindowHandler::~WindowHandler() {
    spdlog::debug("Highest FPS: {}", highest_fps);

    for (const auto& f : m_Framebuffers) {
        kat::destroy(f);
    }
}
