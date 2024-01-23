#include "app.hpp"

namespace kat {
    App::App(const EngineConfig &config) {
        m_engine = Engine::create(config);
    }

    void App::run() {
        m_this_frame = glfwGetTime();
        m_delta_time = 1.0 / 60.0;
        m_last_frame = m_this_frame - m_delta_time;

        while (is_open()) {
            glfwPollEvents();

            update(m_delta_time);

            render_pre(m_delta_time);

            const auto img   = m_engine->get_current_swapchain_image();
            const auto img_v = m_engine->get_current_swapchain_image_view();
            auto       cmd   = m_engine->begin_frame();
            cmd.image_layout_transition(img, kat::color_subresource_range(1, 1),
                                        kat::ImageLayoutState{ vk::PipelineStageFlagBits::eTopOfPipe,
                                                               vk::ImageLayout::eUndefined, vk::AccessFlagBits::eNone },
                                        kat::ImageLayoutState{ vk::PipelineStageFlagBits::eTopOfPipe,
                                                               vk::ImageLayout::eColorAttachmentOptimal,
                                                               vk::AccessFlagBits::eColorAttachmentWrite });

            std::array<float, 4> background_color = { m_background_color.r, m_background_color.g, m_background_color.b,
                                                      1.0f };

            vk::RenderingAttachmentInfo attc{ img_v,
                                              vk::ImageLayout::eColorAttachmentOptimal,
                                              vk::ResolveModeFlagBits::eNone,
                                              {},
                                              vk::ImageLayout::eUndefined,
                                              vk::AttachmentLoadOp::eClear,
                                              vk::AttachmentStoreOp::eStore,
                                              vk::ClearColorValue(background_color) };
            vk::Extent2D                extent = m_engine->get_swapchain_config().extent;

            cmd->beginRendering(vk::RenderingInfo({}, vk::Rect2D({ 0, 0 }, extent), 1, 0, attc));
            render(cmd, m_delta_time);
            cmd->endRendering();

            cmd.image_layout_transition(img, kat::color_subresource_range(1, 1),
                                        kat::ImageLayoutState{ vk::PipelineStageFlagBits::eBottomOfPipe,
                                                               vk::ImageLayout::eColorAttachmentOptimal,
                                                               vk::AccessFlagBits::eColorAttachmentWrite },
                                        kat::ImageLayoutState{ vk::PipelineStageFlagBits::eBottomOfPipe,
                                                               vk::ImageLayout::ePresentSrcKHR,
                                                               vk::AccessFlagBits::eColorAttachmentRead });
            m_engine->end_frame();

            m_last_frame = m_this_frame;
            m_this_frame = glfwGetTime();
            m_delta_time = m_this_frame - m_last_frame;
        }
    }

    bool App::is_open() const {
        return m_open && m_engine->is_open();
    }

    void App::set_background_color(const glm::vec3 &color) {
        m_background_color = color;
    }
} // namespace kat
