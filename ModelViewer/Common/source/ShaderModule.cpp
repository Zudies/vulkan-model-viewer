#include "pch.h"
#include "ShaderModule.h"
#include <fstream>
#include <filesystem>

namespace Graphics {

ShaderModule::ShaderModule() {
}

ShaderModule::~ShaderModule() {
}

std::string const &ShaderModule::GetLastError() const {
    return m_lastError;
}

bool ShaderModule::CreateFromGlsl(std::string const &shaderFile) {
    if (!_readFileToData(shaderFile)) {
        return false;
    }

    //TODO: Verify glsl

    m_lastError.clear();
    return true;
}

bool ShaderModule::CreateFromHlsl(std::string const &shaderFile) {
    if (!_readFileToData(shaderFile)) {
        return false;
    }

    //TODO: Verify hlsl

    m_lastError.clear();
    return true;
}

bool ShaderModule::CreateFromSpirv(std::string const &shaderFile) {
    if (!_readFileToData(shaderFile)) {
        return false;
    }

    //TODO: Verify spv

    m_lastError.clear();
    return true;
}

const uint8_t *ShaderModule::GetData() const {
    return m_data.data();
}

size_t ShaderModule::GetDataSize() const {
    return m_data.size();
}

bool ShaderModule::_readFileToData(std::string const &filepath) {
#ifdef _WIN32
    wchar_t cwd[MAX_PATH];
    auto ret = GetModuleFileName(NULL, cwd, MAX_PATH);
    ASSERT(ret != 0);
    std::filesystem::path exePath(cwd);
    exePath = exePath.parent_path();
#else
#error Not Supported
#endif

    std::ifstream file(exePath /= filepath, std::ios_base::ate | std::ios_base::binary);

    if (!file.is_open()) {
        m_lastError = "Unable to open file";
        return false;
    }

    size_t fileSize = (size_t)file.tellg();
    m_data.resize(fileSize);
    file.seekg(0);
    file.read(reinterpret_cast<char *>(m_data.data()), fileSize);

    file.close();
    m_lastError.clear();

    return true;
}

} // namespace Graphics
