#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace kat {

    struct ImageLayoutState {
        vk::PipelineStageFlags pipeline_stage;
        vk::ImageLayout layout;
        vk::AccessFlags access_flags = vk::AccessFlagBits::eNone;
        uint32_t queue_family;
    };

    class CommandRecorder {
    public:
        CommandRecorder(const vk::CommandBuffer &cmd);

        ~CommandRecorder() = default;

        inline vk::CommandBuffer       *operator->() { return &m_cmd; }
        inline const vk::CommandBuffer *operator->() const { return &m_cmd; }

        inline vk::CommandBuffer       *operator*() { return &m_cmd; }
        inline const vk::CommandBuffer *operator*() const { return &m_cmd; }


        void image_layout_transition(vk::Image image, vk::ImageSubresourceRange subresource_range, ImageLayoutState from_state, ImageLayoutState to_state) const;

    private:
        vk::CommandBuffer m_cmd;
    };

} // namespace kat
