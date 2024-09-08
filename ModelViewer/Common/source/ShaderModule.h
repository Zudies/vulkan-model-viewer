#pragma once

namespace Graphics {

class ShaderModule {
public:
    typedef std::vector<uint8_t> DataArray;

public:
    ShaderModule();
    ShaderModule(ShaderModule const &) = delete;
    ShaderModule &operator=(ShaderModule const &) = delete;
    ~ShaderModule();

    std::string const &GetLastError() const;

    void CreateFromGlsl(std::string const &shaderFile);
    void CreateFromHlsl(std::string const &shaderFile);
    void CreateFromSpirv(std::string const &shaderFile);

    const uint8_t *GetData() const;
    size_t GetDataSize() const;

private:
    void _readFileToData(std::string const &filepath);

private:
    DataArray m_data;
    std::string m_lastError;

};

} // namespace Graphics
