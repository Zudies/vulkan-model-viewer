#include "pch.h"
#include "ShaderModule.h"
#include <fstream>

namespace Graphics {

ShaderModule::ShaderModule() {
}

ShaderModule::~ShaderModule() {
}

std::string const &ShaderModule::GetLastError() const {
    return m_lastError;
}

void ShaderModule::CreateFromGlsl(std::string const &shaderFile) {
    _readFileToData(shaderFile);
    if (!m_lastError.empty()) {
        return;
    }

    //TODO: Verify glsl

    m_lastError.clear();
}

void ShaderModule::CreateFromHlsl(std::string const &shaderFile) {
    _readFileToData(shaderFile);
    if (!m_lastError.empty()) {
        return;
    }

    //TODO: Verify hlsl

    m_lastError.clear();
}

void ShaderModule::CreateFromSpirv(std::string const &shaderFile) {
    _readFileToData(shaderFile);
    if (!m_lastError.empty()) {
        return;
    }

    //TODO: Verify spv

    m_lastError.clear();
}

const uint8_t *ShaderModule::GetData() const {
    return m_data.data();
}

size_t ShaderModule::GetDataSize() const {
    return m_data.size();
}

void ShaderModule::_readFileToData(std::string const &filepath) {
    std::ifstream file(filepath, std::ios_base::ate | std::ios_base::binary);

    if (!file.is_open()) {
        m_lastError = "Unable to open file";
        return;
    }

    size_t fileSize = (size_t)file.tellg();
    m_data.resize(fileSize);
    file.seekg(0);
    file.read(reinterpret_cast<char *>(m_data.data()), fileSize);

    file.close();
    m_lastError.clear();
}

} // namespace Graphics
