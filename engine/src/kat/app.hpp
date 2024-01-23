#pragma once
#include "engine.hpp"

namespace kat {

    class App {
    public:
        App(const EngineConfig &config);

        virtual ~App() = default;

        void run();

        virtual void update(double delta)                           = 0;
        virtual void render(CommandRecorder recorder, double delta) = 0;
        virtual void render_pre(double delta) {}

        // ===== Begin Useful Api Shit ===== //

        [[nodiscard]] bool is_open() const;

        void set_background_color(const glm::vec3 &color);

    private:
        double m_last_frame;
        double m_this_frame;
        double m_delta_time;

        bool m_open = true;

        glm::vec3 m_background_color{ 0.0f, 0.0f, 0.0f };

        std::shared_ptr<Engine> m_engine;
    };

} // namespace kat
