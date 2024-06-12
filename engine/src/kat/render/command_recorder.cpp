#include "command_recorder.hpp"

namespace kat {
    CommandRecorder::CommandRecorder(vk::CommandBuffer commandBuffer) : m_CommandBuffer(commandBuffer) {
    }

    void CommandRecorder::beginPrimary(const cmd::BeginOptions &options) {
        vk::CommandBufferUsageFlags flags{};
        if (options.oneTimeSubmit) flags |= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        if (options.simultaneousUse) flags |= vk::CommandBufferUsageFlagBits::eSimultaneousUse;
        m_CommandBuffer.begin(vk::CommandBufferBeginInfo(flags, nullptr));
    }

    void CommandRecorder::beginSecondary(const cmd::BeginOptions &options, const cmd::SecondaryBeginOptions &secondaryBeginOptions) {
        vk::CommandBufferUsageFlags flags{};
        if (options.oneTimeSubmit) flags |= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        if (options.simultaneousUse) flags |= vk::CommandBufferUsageFlagBits::eSimultaneousUse;
        if (secondaryBeginOptions.renderPassContinue) flags |= vk::CommandBufferUsageFlagBits::eRenderPassContinue;
        m_CommandBuffer.begin(vk::CommandBufferBeginInfo(flags, &secondaryBeginOptions.inheritanceInfo));
    }

    void CommandRecorder::beginRenderPass(const std::shared_ptr<kat::RenderPass> &renderPass, const cmd::RenderPassBeginInfo &renderPassBeginInfo) {
        m_CommandBuffer.beginRenderPass2(vk::RenderPassBeginInfo(renderPass->get(), renderPassBeginInfo.framebuffer, renderPassBeginInfo.renderArea, renderPassBeginInfo.clearValues), vk::SubpassBeginInfo(renderPassBeginInfo.subpassContents));
    }

    void CommandRecorder::endRenderPass() {
        m_CommandBuffer.endRenderPass2(vk::SubpassEndInfo());
    }

    void CommandRecorder::executeCommands(const std::vector<vk::CommandBuffer> &commandBuffers) {
        m_CommandBuffer.executeCommands(commandBuffers);
    }
} // namespace kat