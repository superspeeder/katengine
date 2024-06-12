#pragma once

#include <vulkan/vulkan.hpp>

#include "kat/stack.hpp"

namespace kat::vku {
    struct MemoryStageReference {
        vk::PipelineStageFlags2 stageMask = vk::PipelineStageFlagBits2::eNone;
        vk::AccessFlags2 accessMask = vk::AccessFlagBits2::eNone;
    };

    struct MemoryBarrier {
        MemoryStageReference source, destination;

        [[nodiscard]] inline vk::MemoryBarrier2 desc() const noexcept { return vk::MemoryBarrier2(source.stageMask, source.accessMask, destination.stageMask, destination.accessMask); }
    };

    struct DeviceRegion {
        vk::DeviceSize offset, size;
    };

    struct BufferMemoryBarrier {
        MemoryStageReference source, destination;
        uint32_t sourceQueueFamily, destinationQueueFamily;
        vk::Buffer buffer;
        DeviceRegion region;

        [[nodiscard]] inline vk::BufferMemoryBarrier2 desc() const noexcept {
            return vk::BufferMemoryBarrier2(source.stageMask, source.accessMask, destination.stageMask, destination.accessMask, sourceQueueFamily, destinationQueueFamily, buffer, region.offset, region.size);
        };
    };

    struct ImageMemoryBarrier {
        MemoryStageReference source, destination;
        uint32_t sourceQueueFamily, destinationQueueFamily;
        vk::ImageLayout oldLayout, newLayout;
        vk::Image image;
        vk::ImageSubresourceRange subresourceRange;

        [[nodiscard]] inline vk::ImageMemoryBarrier2 desc() const noexcept {
            return vk::ImageMemoryBarrier2(source.stageMask, source.accessMask, destination.stageMask, destination.accessMask, oldLayout, newLayout, sourceQueueFamily, destinationQueueFamily, image, subresourceRange);
        };
    };

    struct DependencyInfo {
        vk::DependencyFlags dependencyFlags;
        std::vector<MemoryBarrier> memoryBarriers;
        std::vector<BufferMemoryBarrier> bufferMemoryBarriers;
        std::vector<ImageMemoryBarrier> imageMemoryBarriers;

        [[nodiscard]] inline vk::DependencyInfo desc(kat::stack &stack) const {
            auto *mbs = stack.smalloc<vk::MemoryBarrier2>(memoryBarriers.size());
            auto *bmbs = stack.smalloc<vk::BufferMemoryBarrier2>(bufferMemoryBarriers.size());
            auto *imbs = stack.smalloc<vk::ImageMemoryBarrier2>(imageMemoryBarriers.size());

            for (size_t i = 0; i < memoryBarriers.size(); i++) mbs[i] = memoryBarriers[i].desc();
            for (size_t i = 0; i < bufferMemoryBarriers.size(); i++) bmbs[i] = bufferMemoryBarriers[i].desc();
            for (size_t i = 0; i < imageMemoryBarriers.size(); i++) imbs[i] = imageMemoryBarriers[i].desc();

            return {dependencyFlags, static_cast<uint32_t>(memoryBarriers.size()), mbs, static_cast<uint32_t>(bufferMemoryBarriers.size()), bmbs, static_cast<uint32_t>(imageMemoryBarriers.size()), imbs};
        };
    };
} // namespace kat::vku