#pragma once

#include "ShaderModule.h"

namespace Vulkan {

class RendererImpl;

class VulkanShaderModule {
public:
    VulkanShaderModule(RendererImpl *parentRenderer);
    VulkanShaderModule(VulkanShaderModule const &) = delete;
    VulkanShaderModule &operator=(VulkanShaderModule const &) = delete;
    ~VulkanShaderModule();

    void SetShaderStage(VkShaderStageFlagBits shaderStage);
    VkShaderStageFlagBits GetShaderStage() const;

    std::string const &GetLastError() const;

    void CreateFromGlsl(std::string const &shaderFile);
    void CreateFromHlsl(std::string const &shaderFile);
    void CreateFromSpirv(std::string const &shaderFile);

    // Only needed if not already created from SPIR-V
    void Compile(std::string const &compilerArgs);

    VkShaderModule GetShaderModule() const;

private:
    void _createVkShaderModule(const uint8_t *data, size_t dataSize);

private:
    RendererImpl *m_renderer;
    Graphics::ShaderModule m_source;
    VkShaderModule m_shaderModule;
    VkShaderStageFlagBits m_shaderStage;
    std::string m_lastError;

};

} // namespace Vulkan
