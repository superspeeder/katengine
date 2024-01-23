#include "render_pipeline.hpp"

#include <utility>

namespace kat {
    ShaderModule::ShaderModule(const std::shared_ptr<Engine> &engine, const std::vector<uint32_t> &code,
                               vk::ShaderStageFlagBits stage, std::string entry_point) :
        m_stage(stage), m_entry_point(std::move(entry_point)) {
        m_module = engine->get_device().createShaderModule(vk::ShaderModuleCreateInfo({}, code));
    }

    ShaderModule::~ShaderModule() {
        m_engine->get_device().destroy(m_module);
    }


} // namespace kat
