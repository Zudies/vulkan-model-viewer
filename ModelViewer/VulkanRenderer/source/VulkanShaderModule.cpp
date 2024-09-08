#include "pch.h"
#include "VulkanShaderModule.h"
#include "VulkanRendererImpl.h"

namespace Vulkan {

VulkanShaderModule::VulkanShaderModule(RendererImpl *parentRenderer)
  : m_renderer(parentRenderer),
    m_shaderModule(VK_NULL_HANDLE) {
}

VulkanShaderModule::~VulkanShaderModule() {
    if (m_shaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(m_renderer->m_device, m_shaderModule, VK_NULL_HANDLE);
    }
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

    m_lastError.clear();

    // Already spirv so create shader module
    _createVkShaderModule(m_source.GetData(), m_source.GetDataSize());
}

void VulkanShaderModule::Compile(std::string const &compilerArgs) {
    if (m_shaderModule) {
        m_lastError = "Shader module already created";
        return;
    }

    //TODO: load libshaderc and compile
    std::vector<uint8_t> compiledData;

    // Create shader module
    _createVkShaderModule(compiledData.data(), compiledData.size());
}

void VulkanShaderModule::_createVkShaderModule(const uint8_t *data, size_t dataSize) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = dataSize;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(data);

    if (vkCreateShaderModule(m_renderer->m_device, &createInfo, nullptr, &m_shaderModule) != VK_SUCCESS) {
        m_lastError = "Failed to create vkShaderModule";
    }
}

VkShaderModule VulkanShaderModule::GetShaderModule() const {
    return m_shaderModule;
}

} // namespace Vulkan
