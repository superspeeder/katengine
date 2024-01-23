#pragma once
#include "kat/app.hpp"
#include "kat/engine.hpp"

namespace sample {
    class SampleApp : public kat::App {
    public:
        SampleApp();

        virtual ~SampleApp() = default;

        void update(double delta) override;
        void render(kat::CommandRecorder recorder, double delta) override;

        void render_pre(double delta) override;

    private:
        std::shared_ptr<kat::Engine> m_engine;
        float                        h = 0.0f;
    };
} // namespace sample
