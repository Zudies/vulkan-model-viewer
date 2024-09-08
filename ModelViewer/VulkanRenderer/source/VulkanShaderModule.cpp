#include "pch.h"
#include "VulkanShaderModule.h"

namespace Vulkan {

VulkanShaderModule::VulkanShaderModule() {
}

VulkanShaderModule::~VulkanShaderModule() {
}

std::string const &VulkanShaderModule::GetLastError() const {
    return m_lastError;
}

void VulkanShaderModule::CreateFromGlsl(std::string const &shaderFile) {
    m_source.CreateFromGlsl(shaderFile);
    if (!m_source.GetLastError().empty()) {
        m_lastError = m_source.GetLastError();
        return;
    }

    m_lastError.clear();
}

void VulkanShaderModule::CreateFromHlsl(std::string const &shaderFile) {
    m_source.CreateFromHlsl(shaderFile);
    if (!m_source.GetLastError().empty()) {
        m_lastError = m_source.GetLastError();
        return;
    }

    m_lastError.clear();
}

void VulkanShaderModule::CreateFromSpirv(std::string const &shaderFile) {
    m_source.CreateFromSpirv(shaderFile);
    if (!m_source.GetLastError().empty()) {
        m_lastError = m_source.GetLastError();
        return;
    }

    // Already spirv so swap the data over
    m_spirvData.swap(m_source.GetInternalData());

    m_lastError.clear();
}

void VulkanShaderModule::Compile(std::string const &compilerArgs) {
    if (!m_spirvData.empty()) {
        m_lastError = "Shader module already compiled to SPIR-V";
        return;
    }

    //TODO: load libshaderc and compile
}

const uint8_t *VulkanShaderModule::GetData() const {
    return m_spirvData.data();
}

} // namespace Vulkan
