#pragma once

#include <kat/render/render_pass.hpp>
#include <kat/engine.hpp>
#include <kat/window.hpp>

class WindowHandler : public kat::BaseWindowHandler {
  public:
    explicit WindowHandler(kat::Window* window);
    virtual ~WindowHandler();


    void onRender(const std::shared_ptr<kat::Window> &window, const kat::WindowFrameResources &resources) override;

  private:

    kat::Window* m_Window;

    std::shared_ptr<kat::RenderPass> m_RenderPass;
    std::vector<vk::Framebuffer> m_Framebuffers;

    double lastFrame;
    double thisFrame;
    double delta;

    double highest_fps = 0.0f;

    size_t fcounter = 0;
};
