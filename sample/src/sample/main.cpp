#include "sample/main.hpp"

#include <glm/gtx/color_space.hpp>

namespace sample {
    const kat::EngineConfig ENGINE_CONFIG{ kat::WindowConfig{ "Window", { 800, 600 } } };

    SampleApp::SampleApp() : App(ENGINE_CONFIG) {}

    void SampleApp::update(double delta) {
        h = glm::fract(h + 0.1 * delta);
    }

    void SampleApp::render(kat::CommandRecorder recorder, double delta) {}

    void SampleApp::render_pre(double delta) {
        set_background_color(glm::rgbColor(glm::vec3{h * 360.0f, 1.0f, 1.0f}));
    }

} // namespace sample

int main() {
    const auto app = std::make_unique<sample::SampleApp>();
    app->run();
    return 0;
}
