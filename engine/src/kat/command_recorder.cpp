#include "command_recorder.hpp"

namespace kat {
    CommandRecorder::CommandRecorder(const vk::CommandBuffer &cmd) : m_cmd(cmd) {}

    void CommandRecorder::image_layout_transition(vk::Image image, vk::ImageSubresourceRange subresource_range,
                                                  ImageLayoutState from_state, ImageLayoutState to_state) const {

        vk::ImageMemoryBarrier barrier{};
        barrier.image               = image;
        barrier.oldLayout           = from_state.layout;
        barrier.newLayout           = to_state.layout;
        barrier.srcAccessMask       = from_state.access_flags;
        barrier.dstAccessMask       = to_state.access_flags;
        barrier.srcQueueFamilyIndex = from_state.queue_family;
        barrier.dstQueueFamilyIndex = to_state.queue_family;
        barrier.subresourceRange    = subresource_range;
        m_cmd.pipelineBarrier(from_state.pipeline_stage, to_state.pipeline_stage, {}, {}, {}, barrier);
    }
} // namespace kat
