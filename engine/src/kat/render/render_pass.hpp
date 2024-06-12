#pragma once

#include <kat/engine.hpp>

namespace kat {

    struct LoadStoreOps {
        vk::AttachmentLoadOp loadOp;
        vk::AttachmentStoreOp storeOp;
    };

    constexpr LoadStoreOps LSO_STANDARD_CLEAR_STORE = LoadStoreOps{vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore};
    constexpr LoadStoreOps LSO_DONT_CARE = LoadStoreOps{vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare};

    struct AttachmentStencilLayout {
        vk::ImageLayout initialLayout;
        vk::ImageLayout finalLayout;
    };

    struct AttachmentInfo {
        vk::Format format;
        vk::ImageLayout initialLayout;
        vk::ImageLayout finalLayout;

        LoadStoreOps colorDepthLSO = LSO_STANDARD_CLEAR_STORE;
        LoadStoreOps stencilLSO = LSO_DONT_CARE;
        vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;

        std::optional<AttachmentStencilLayout> stencilLayouts = std::nullopt;
    };

    constexpr AttachmentInfo simpleRenderToPresentAttachment(vk::Format format) {
        return AttachmentInfo{format, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR};
    }

    struct AttachmentReference {
        uint32_t attachment;
        vk::ImageLayout layout;
        vk::ImageAspectFlags aspectFlags;
        std::optional<vk::ImageLayout> stencilLayout;
    };

    struct FragmentShadingRateAttachmentReference {
        AttachmentReference attachmentReference;
        vk::Extent2D texelSize;
    };

    struct SubpassMultisampledRenderToSingleInfo {
        bool enable;
        vk::SampleCountFlagBits rasterizationSamples;
    };

    struct RenderPassCreationControlInfo {
        bool disallowMerging;
    };

    struct SubpassDepthStencilResolveInfo {
        vk::ResolveModeFlagBits depthResolveMode;
        vk::ResolveModeFlagBits stencilResolveMode;
        AttachmentReference depthStencilResolveAttachment;
    };

    struct SubpassInfo {
        vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics;
        uint32_t viewMask = 0;

        std::vector<AttachmentReference> inputAttachments;
        std::vector<AttachmentReference> colorAttachments;
        std::vector<AttachmentReference> resolveAttachments;
        std::optional<AttachmentReference> depthStencilAttachment;
        std::vector<uint32_t> preservedAttachments;

        std::optional<FragmentShadingRateAttachmentReference> fragmentShadingRateAttachment;
        std::optional<SubpassMultisampledRenderToSingleInfo> multisampleRenderToSingleInfo;
        std::optional<RenderPassCreationControlInfo> creationControlInfo;
        std::optional<SubpassDepthStencilResolveInfo> depthStencilResolveInfo;
    };

    struct SubpassReference {
        uint32_t subpass;
        vk::PipelineStageFlags stage;
        vk::AccessFlags access;
    };

    struct MemoryBarrier {
        vk::PipelineStageFlags2 sourceStage;
        vk::PipelineStageFlags2 destinationStage;

        vk::AccessFlags2 sourceAccess;
        vk::AccessFlags2 destinationAccess;
    };

    struct SubpassDependency {
        SubpassReference source, destination;

        vk::DependencyFlags dependencyFlags = vk::DependencyFlags{};
        int32_t viewOffset = 0;

        std::optional<MemoryBarrier> memoryBarrier;
    };

    struct RenderPassInfo {
        std::vector<AttachmentInfo> attachments;
        std::vector<SubpassInfo> subpasses;
        std::vector<SubpassDependency> subpassDependencies;
        std::vector<uint32_t> correlatedViewMasks;

        std::optional<AttachmentReference> fragmentDensityMap;
        std::optional<RenderPassCreationControlInfo> creationControlInfo;
    };

    class RenderPass {
      public:

        explicit RenderPass(const RenderPassInfo& info);
        ~RenderPass();

        [[nodiscard]] inline vk::RenderPass get() const noexcept { return m_RenderPass; };

        vk::Framebuffer createCompatibleFramebuffer(const std::vector<vk::ImageView> &attachments, vk::Extent3D extent) const;
        vk::Framebuffer createCompatibleFramebuffer(vk::ImageView &attachment, vk::Extent3D extent) const;

      private:
        vk::RenderPass m_RenderPass;
    };

} // namespace kat
