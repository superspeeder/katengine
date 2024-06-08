#pragma once

#include <kat/engine.hpp>
#include <kat/window.hpp>

class WindowHandler : public kat::BaseWindowHandler {
  public:
    explicit WindowHandler(kat::Window* window);

    void onRender(const std::shared_ptr<kat::Window> &window, const kat::WindowFrameResources &resources) override;

  private:

    kat::Window* m_Window;
};
