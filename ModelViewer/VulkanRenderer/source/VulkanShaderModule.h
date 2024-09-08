#pragma once

#include "ShaderModule.h"

namespace Vulkan {

class VulkanShaderModule {
public:
    VulkanShaderModule();
    VulkanShaderModule(VulkanShaderModule const &) = delete;
    VulkanShaderModule &operator=(VulkanShaderModule const &) = delete;
    ~VulkanShaderModule();

    std::string const &GetLastError() const;

    void CreateFromGlsl(std::string const &shaderFile);
    void CreateFromHlsl(std::string const &shaderFile);
    void CreateFromSpirv(std::string const &shaderFile);

    void Compile(std::string const &compilerArgs);

    const uint8_t *GetData() const;

private:
    Graphics::ShaderModule m_source;
    Graphics::ShaderModule::DataArray m_spirvData;
    std::string m_lastError;

};

} // namespace Vulkan
