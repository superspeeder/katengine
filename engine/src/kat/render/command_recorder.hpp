#pragma once

#include "kat/engine.hpp"
#include "kat/render/render_pass.hpp"

namespace kat {

    namespace cmd {
        struct BeginOptions {
            bool oneTimeSubmit = false;
            bool simultaneousUse = false;
        };

        struct SecondaryBeginOptions {
            bool renderPassContinue = false;

            vk::CommandBufferInheritanceInfo inheritanceInfo = {};
        };

        struct RenderPassBeginInfo {
            vk::Framebuffer framebuffer;
            std::vector<vk::ClearValue> clearValues;
            vk::Rect2D renderArea;

            vk::SubpassContents subpassContents = vk::SubpassContents::eInline;
        };
    } // namespace cmd

    class CommandRecorder {
      public:
        explicit CommandRecorder(vk::CommandBuffer commandBuffer);
        ~CommandRecorder() = default;

        void beginPrimary(const cmd::BeginOptions &options = {});
        void beginSecondary(const cmd::BeginOptions &options = {}, const cmd::SecondaryBeginOptions &secondaryBeginOptions = {});

        void beginRenderPass(const std::shared_ptr<kat::RenderPass> &renderPass, const cmd::RenderPassBeginInfo &renderPassBeginInfo);
        void endRenderPass();

        void executeCommands(const std::vector<vk::CommandBuffer> &commandBuffers);

        void setEvent(const vk::Event &event, const vku::DependencyInfo &dependencyInfo = {});
        void resetEvent(const vk::Event &event, vk::PipelineStageFlags2 stageFlags);
        void waitEvents(const std::vector<vk::Event> &events, const vku::DependencyInfo &dependencyInfo = {});

        void pipelineBarrier(const vku::DependencyInfo &dependencyInfo = {});

        inline const vk::CommandBuffer *operator->() const noexcept { return &m_CommandBuffer; };

        inline const vk::CommandBuffer &operator*() const noexcept { return m_CommandBuffer; };

        [[nodiscard]] inline const vk::CommandBuffer &get() const noexcept { return m_CommandBuffer; };

      private:
        vk::CommandBuffer m_CommandBuffer;
    };

} // namespace kat
