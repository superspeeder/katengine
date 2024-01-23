#pragma once

#include "kat/engine.hpp"

namespace kat {

    class ShaderModule {
    public:
        ShaderModule(const std::shared_ptr<Engine> &engine, const std::vector<uint32_t> &code,
                     vk::ShaderStageFlagBits stage, std::string entry_point = "main");

        ~ShaderModule();

        [[nodiscard]] const vk::ShaderModule &get_module() const { return m_module; }
        [[nodiscard]] vk::ShaderStageFlagBits get_stage() const { return m_stage; }
        [[nodiscard]] const std::string      &get_entry_point() const { return m_entry_point; }

    private:
        std::shared_ptr<Engine> m_engine;
        vk::ShaderModule        m_module;
        vk::ShaderStageFlagBits m_stage;
        std::string             m_entry_point;
    };

    class PipelineLayout {
        // TODO
    };

    struct VertexAttributeLayout {
        uint32_t   location;
        vk::Format format;
        uint32_t   offset;
    };

    struct VertexBindingLayout {
        uint32_t                           binding;
        uint32_t                           stride;
        std::vector<VertexAttributeLayout> attributes;

        vk::VertexInputRate input_rate = vk::VertexInputRate::eVertex;
    };

    struct VertexLayout {
        std::vector<VertexBindingLayout> bindings;
    };

    struct InputAssemblySettings {
        vk::PrimitiveTopology topology               = vk::PrimitiveTopology::eTriangleList;
        bool                  enablePrimitiveRestart = false;
    };

    struct TessellatorSettings {
        uint32_t patchControlPoints = 1;
    };

    struct ViewportsAndScissors {
        std::vector<vk::Viewport> viewports;
        std::vector<vk::Rect2D>   scissors;
    };

    struct RasterizationSettings {
        vk::PolygonMode   polygon_mode            = vk::PolygonMode::eFill;
        vk::CullModeFlags cull_mode               = vk::CullModeFlagBits::eNone;
        vk::FrontFace     front_face              = vk::FrontFace::eCounterClockwise;
        bool              enableDepthClamp        = false;
        bool              discardRasterizerOutput = false;
        bool              enableDepthBias         = false;
        float             depthBiasConstantFactor = 0.0f;
        float             depthBiasClamp          = 0.0f;
        float             depthBiasSlopeFactor    = 0.0f;
        float             lineWidth               = 1.0f;
    };

    struct MultisampleSettings {
        vk::SampleCountFlagBits     samples             = vk::SampleCountFlagBits::e1;
        bool                        enableSampleShading = false;
        float                       minSampleShading    = 0.0f;
        std::vector<vk::SampleMask> sample_mask;
        bool                        enableAlphaToCoverage = false;
        bool                        enableAlphaToOne      = false;
    };

    struct DepthStencilSettings {
        bool               enableDepthTest       = false;
        bool               enableDepthWrite      = false;
        vk::CompareOp      depthCompareOp        = vk::CompareOp::eLess;
        bool               enableDepthBoundsTest = false;
        bool               enableStencilTest     = false;
        vk::StencilOpState frontStencil{};
        vk::StencilOpState backStencil{};
        float              minDepthBounds = 0.0f;
        float              maxDepthBounds = 0.0f;
    };

    struct BlendAttachment {
        bool enable = true;

        vk::BlendFactor src_color = vk::BlendFactor::eSrcAlpha;
        vk::BlendFactor dst_color = vk::BlendFactor::eOneMinusSrcColor;
        vk::BlendOp     color_op  = vk::BlendOp::eAdd;

        vk::BlendFactor src_alpha = vk::BlendFactor::eOne;
        vk::BlendFactor dst_alpha = vk::BlendFactor::eZero;
        vk::BlendOp     alpha_op  = vk::BlendOp::eAdd;

        vk::ColorComponentFlags color_write_mask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    };

    struct BlendSettings {
        bool                         enableLogicOp = false;
        vk::LogicOp                  logic_op      = vk::LogicOp::eSet;
        std::vector<BlendAttachment> attachments;
        std::array<float, 4>         blend_constants{ 0.0f, 0.0f, 0.0f, 0.0f };
    };

    struct PipelineSettings {
        std::vector<std::shared_ptr<ShaderModule>> shaders;
        VertexLayout                               vertex_layout{};
        InputAssemblySettings                      input_assembly_settings{};
        TessellatorSettings                        tessellator_settings{};
        ViewportsAndScissors                       viewports_and_scissors{};
        RasterizationSettings                      rasterization_settings{};
        MultisampleSettings                        multisample_settings{};
        DepthStencilSettings                       depth_stencil_settings{};
        BlendSettings                              blend_settings{};
        std::vector<vk::DynamicState>              dynamic_states;
        std::shared_ptr<PipelineLayout>            layout;
        vk::RenderPass                             render_pass{}; // TODO: promote to its own object.
        uint32_t                                   subpass = 0;
    };

    class RenderPipeline {
    public:
        RenderPipeline(const std::shared_ptr<Engine> &engine);

        ~RenderPipeline();

    private:
        std::shared_ptr<Engine> m_engine;
        vk::Pipeline            m_pipeline;
    };

} // namespace kat
