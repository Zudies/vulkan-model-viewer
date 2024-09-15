#pragma once

namespace Graphics {

class ImageLoader {
public:
    typedef std::vector<uint8_t> DataArray;

public:

    ImageLoader();
    ~ImageLoader();

    std::string const &GetLastError() const;

    // When loading from file, the image will be loaded with 4 channels (rgba) per pixel
    bool LoadImageFromFile(std::string const &filePath, uint32_t desiredChannels);
    bool LoadImageFromMemory(void *data, size_t dataSize, uint32_t desiredChannels);

    const void *GetData() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    uint32_t GetDepth() const;
    uint32_t GetChannels() const; // Number of 8-bit channels per pixel

private:
    DataArray m_data;
    uint32_t m_width, m_height, m_depth, m_channels;
    std::string m_lastError;

};

} // namespace Graphics
