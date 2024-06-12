//
// Created by andy on 6/7/24.
//

#include "render_pass.hpp"

namespace {
    //    struct SPII_ {
    //        std::vector<vk::AttachmentReference2> inputAttachments;
    //        std::vector<vk::AttachmentReference2> colorAttachments;
    //        std::vector<vk::AttachmentReference2> resolveAttachments;
    //        vk::AttachmentReference2 dsAttachment;
    //        vk::AttachmentReference2 fsrAttachment;
    //        vk::FragmentShadingRateAttachmentInfoKHR fsr;
    //        vk::MultisampledRenderToSingleSampledInfoEXT msrtss;
    //        vk::RenderPassCreationControlEXT rpcc;
    //        vk::SubpassDescriptionDepthStencilResolve dsr;
    //        vk::AttachmentReference2 dsrAttachment;
    //    };
}; // namespace

namespace kat {
    RenderPass::RenderPass(const RenderPassInfo &info) {
        kat::stack st;

        std::vector<vk::AttachmentDescription2> attachments;

        for (const auto &aInfo: info.attachments) {
            vk::AttachmentDescription2 desc{};
            desc.setInitialLayout(aInfo.initialLayout).setFinalLayout(aInfo.finalLayout);
            desc.setLoadOp(aInfo.colorDepthLSO.loadOp).setStoreOp(aInfo.colorDepthLSO.storeOp);
            desc.setStencilLoadOp(aInfo.stencilLSO.loadOp).setStencilStoreOp(aInfo.stencilLSO.storeOp);
            desc.setFormat(aInfo.format).setSamples(aInfo.sampleCount);

            attachments.push_back(desc);
            if (aInfo.stencilLayouts.has_value()) {
                auto *adsl = st.smalloc(vk::AttachmentDescriptionStencilLayout(aInfo.stencilLayouts->initialLayout, aInfo.stencilLayouts->finalLayout));
                desc.setPNext(adsl);
            }
        }

        std::vector<vk::SubpassDescription2> subpasses;

        for (const auto &sInfo: info.subpasses) {
            auto *inputAttachments = st.smalloc<vk::AttachmentReference2>(sInfo.inputAttachments.size());
            size_t index = 0;
            for (const auto &ar: sInfo.inputAttachments) {
                if (ar.stencilLayout.has_value()) {
                    auto *arsl = st.smalloc<vk::AttachmentReferenceStencilLayout>();
                    arsl->stencilLayout = ar.stencilLayout.value();
                    inputAttachments[index] = vk::AttachmentReference2(ar.attachment, ar.layout, ar.aspectFlags, arsl);
                } else {
                    inputAttachments[index] = vk::AttachmentReference2(ar.attachment, ar.layout, ar.aspectFlags, nullptr);
                }
            }

            index = 0;
            auto *colorAttachments = st.smalloc<vk::AttachmentReference2>(sInfo.colorAttachments.size());
            for (const auto &ar: sInfo.colorAttachments) {
                if (ar.stencilLayout.has_value()) {
                    auto *arsl = st.smalloc<vk::AttachmentReferenceStencilLayout>();
                    arsl->stencilLayout = ar.stencilLayout.value();
                    colorAttachments[index] = vk::AttachmentReference2(ar.attachment, ar.layout, ar.aspectFlags, arsl);
                } else {
                    colorAttachments[index] = vk::AttachmentReference2(ar.attachment, ar.layout, ar.aspectFlags, nullptr);
                }
            }

            index = 0;
            auto *resolveAttachments = st.smalloc<vk::AttachmentReference2>(sInfo.resolveAttachments.size());
            for (const auto &ar: sInfo.resolveAttachments) {
                if (ar.stencilLayout.has_value()) {
                    auto *arsl = st.smalloc<vk::AttachmentReferenceStencilLayout>();
                    arsl->stencilLayout = ar.stencilLayout.value();
                    resolveAttachments[index] = vk::AttachmentReference2(ar.attachment, ar.layout, ar.aspectFlags, arsl);
                } else {
                    resolveAttachments[index] = vk::AttachmentReference2(ar.attachment, ar.layout, ar.aspectFlags, nullptr);
                }
            }

            vk::AttachmentReference2 *dsa = nullptr;

            if (sInfo.depthStencilAttachment.has_value()) {
                dsa = st.smalloc(vk::AttachmentReference2(sInfo.depthStencilAttachment->attachment, sInfo.depthStencilAttachment->layout, sInfo.depthStencilAttachment->aspectFlags, nullptr));
                if (sInfo.depthStencilAttachment->stencilLayout.has_value()) {
                    auto *dsasl = st.smalloc(vk::AttachmentReferenceStencilLayout(sInfo.depthStencilAttachment->stencilLayout.value()));
                    dsa->pNext = dsasl;
                }
            }

            vk::AttachmentReference2 *fsra = nullptr;
            vk::FragmentShadingRateAttachmentInfoKHR *fsri = nullptr;

            if (sInfo.fragmentShadingRateAttachment.has_value()) {
                fsra = st.smalloc(vk::AttachmentReference2(sInfo.depthStencilAttachment->attachment, sInfo.depthStencilAttachment->layout, sInfo.depthStencilAttachment->aspectFlags, nullptr));
                if (sInfo.fragmentShadingRateAttachment->attachmentReference.stencilLayout.has_value()) {
                    auto *fsrasl = st.smalloc(vk::AttachmentReferenceStencilLayout(sInfo.fragmentShadingRateAttachment->attachmentReference.stencilLayout.value()));
                    fsra->pNext = fsrasl;
                }

                fsri = st.smalloc(vk::FragmentShadingRateAttachmentInfoKHR(fsra, sInfo.fragmentShadingRateAttachment->texelSize));
            }

            vk::MultisampledRenderToSingleSampledInfoEXT *mrtssi = nullptr;

            if (sInfo.multisampleRenderToSingleInfo.has_value()) {
                mrtssi = st.smalloc(vk::MultisampledRenderToSingleSampledInfoEXT(sInfo.multisampleRenderToSingleInfo->enable, sInfo.multisampleRenderToSingleInfo->rasterizationSamples));
            }

            vk::RenderPassCreationControlEXT *rpcc = nullptr;

            if (sInfo.creationControlInfo.has_value()) {
                rpcc = st.smalloc(vk::RenderPassCreationControlEXT(sInfo.creationControlInfo->disallowMerging));
            }

            vk::SubpassDescriptionDepthStencilResolve *dsr = nullptr;
            vk::AttachmentReference2 *dsra = nullptr;

            if (sInfo.depthStencilResolveInfo.has_value()) {
                dsra = st.smalloc(vk::AttachmentReference2(sInfo.depthStencilResolveInfo->depthStencilResolveAttachment.attachment, sInfo.depthStencilResolveInfo->depthStencilResolveAttachment.layout, sInfo.depthStencilResolveInfo->depthStencilResolveAttachment.aspectFlags, nullptr));
                if (sInfo.depthStencilResolveInfo->depthStencilResolveAttachment.stencilLayout.has_value()) {
                    auto *dsrasl = st.smalloc(vk::AttachmentReferenceStencilLayout(sInfo.depthStencilResolveInfo->depthStencilResolveAttachment.stencilLayout.value()));
                    dsra->pNext = dsrasl;
                }
            }

            // do important things
            assert(sInfo.resolveAttachments.size() == 0 || sInfo.resolveAttachments.size() == sInfo.colorAttachments.size());
            if (sInfo.resolveAttachments.size() == 0) {
                resolveAttachments = nullptr; // if any memory was allocated to a 0-length array (idk why it would be but who knows), itll get cleaned up with the rest but not used.
            }

            DynamicStructureChain dchain;
            dchain.push(fsri);
            dchain.push(mrtssi);
            dchain.push(rpcc);
            dchain.push(dsr);

            subpasses.emplace_back(vk::SubpassDescriptionFlags(/* TODO */), sInfo.bindPoint, sInfo.viewMask, sInfo.inputAttachments.size(), inputAttachments, sInfo.colorAttachments.size(), colorAttachments, resolveAttachments, dsa, sInfo.preservedAttachments.size(), sInfo.preservedAttachments.data(), dchain.get());
        }

        std::vector<vk::SubpassDependency2> subpassDependencies;
        for (const auto &sd: info.subpassDependencies) {
            void *mb = nullptr;
            if (sd.memoryBarrier.has_value()) {
                const auto &memoryBarrier = *sd.memoryBarrier;
                mb = st.smalloc(vk::MemoryBarrier2(memoryBarrier.sourceStage, memoryBarrier.sourceAccess, memoryBarrier.destinationStage, memoryBarrier.destinationAccess));
            }

            subpassDependencies.emplace_back(sd.source.subpass, sd.destination.subpass, sd.source.stage, sd.destination.stage, sd.source.access, sd.destination.access, sd.dependencyFlags, sd.viewOffset, mb);
        }

        DynamicStructureChain rpdsc;

        if (info.fragmentDensityMap.has_value()) {
            rpdsc.push(st.smalloc(vk::RenderPassFragmentDensityMapCreateInfoEXT(vk::AttachmentReference(info.fragmentDensityMap->attachment, info.fragmentDensityMap->layout))));
        }

        if (info.creationControlInfo.has_value()) {
            rpdsc.push(st.smalloc(vk::RenderPassCreationControlEXT(info.creationControlInfo->disallowMerging)));
        }

        m_RenderPass = globalState->device.createRenderPass2(vk::RenderPassCreateInfo2({/* TODO: flags */}, attachments, subpasses, subpassDependencies, info.correlatedViewMasks, rpdsc.get()));
    }

    RenderPass::~RenderPass() {
        destroy(m_RenderPass);
    }

    vk::Framebuffer RenderPass::createCompatibleFramebuffer(const std::vector<vk::ImageView> &attachments, vk::Extent3D extent) const {
        return globalState->device.createFramebuffer(vk::FramebufferCreateInfo(vk::FramebufferCreateFlags(), m_RenderPass, attachments, extent.width, extent.height, extent.depth));
    }

    vk::Framebuffer RenderPass::createCompatibleFramebuffer(vk::ImageView &attachment, vk::Extent3D extent) const {
        return globalState->device.createFramebuffer(vk::FramebufferCreateInfo(vk::FramebufferCreateFlags(), m_RenderPass, attachment, extent.width, extent.height, extent.depth));
    }
} // namespace kat