#include "pch.h"
#include "ImageLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <filesystem>

namespace Graphics {

ImageLoader::ImageLoader()
  : m_width(0),
    m_height(0),
    m_depth(0),
    m_channels(0) {

}

ImageLoader::~ImageLoader() {

}

std::string const &ImageLoader::GetLastError() const {
    return m_lastError;
}

bool ImageLoader::LoadImageFromFile(std::string const &filePath, uint32_t desiredChannels) {
#ifdef _WIN32
    wchar_t cwd[MAX_PATH];
    auto ret = GetModuleFileName(NULL, cwd, MAX_PATH);
    ASSERT(ret != 0);
    std::filesystem::path exePath(cwd);
    exePath = exePath.parent_path();
#else
#error Not Supported
#endif

    int width, height, channels;
    exePath /= filePath;
    stbi_uc *imageData = stbi_load(exePath.u8string().c_str(), &width, &height, &channels, desiredChannels);

    if (!imageData) {
        m_lastError = stbi_failure_reason();
        return false;
    }

    m_data.resize(width * height * desiredChannels);
    memcpy(m_data.data(), imageData, m_data.size());

    m_width = width;
    m_height = height;
    m_depth = 1;
    m_channels = desiredChannels;

    stbi_image_free(imageData);

    m_lastError.clear();
    return true;
}

bool ImageLoader::LoadImageFromMemory(void *data, size_t dataSize, uint32_t desiredChannels) {
    int width, height, channels;
    stbi_uc *imageData = stbi_load_from_memory(reinterpret_cast<stbi_uc*>(data), static_cast<int>(dataSize), &width, &height, &channels, desiredChannels);

    if (!imageData) {
        m_lastError = stbi_failure_reason();
        return false;
    }

    m_data.resize(width * height * desiredChannels);
    memcpy(m_data.data(), imageData, m_data.size());

    m_width = width;
    m_height = height;
    m_depth = 1;
    m_channels = desiredChannels;

    stbi_image_free(imageData);

    m_lastError.clear();
    return true;
}

const void *ImageLoader::GetData() const {
    return m_data.data();
}

uint32_t ImageLoader::GetWidth() const {
    return m_width;
}

uint32_t ImageLoader::GetHeight() const {
    return m_height;
}

uint32_t ImageLoader::GetDepth() const {
    return m_depth;
}

uint32_t ImageLoader::GetChannels() const {
    return m_channels;
}

} // namespace Graphics
