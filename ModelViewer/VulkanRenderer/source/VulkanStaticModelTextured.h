#pragma once

#include "Transform.h"
#include "VulkanVertexBuffer.h"
#include "Vulkan2DTextureBuffer.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanDescriptorSetInstance.h"

namespace Vulkan {

class RendererImpl;

// A static model that is textured
// Each vertex contains a 3D position, normal, RGB color, and UV texture coordinates
struct VulkanTexturedVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();
};

class VulkanStaticModelTextured {
public:
    VulkanStaticModelTextured(RendererImpl *renderer);
    VulkanStaticModelTextured(VulkanStaticModelTextured const &) = delete;
    VulkanStaticModelTextured &operator=(VulkanStaticModelTextured const &) = delete;
    ~VulkanStaticModelTextured();

    Graphics::GraphicsError LoadFromObjFile(std::string const &objFilePath);

    Graphics::GraphicsError Draw(f64 deltaTime);

private:
    VulkanVertexBuffer<VulkanTexturedVertex> m_vertexData;
    std::vector<Vulkan2DTextureBuffer> m_materialData;
    VulkanDescriptorSetLayout m_descriptorSetLayout;
    VulkanDescriptorSetInstance m_descriptorSet;
    Graphics::Transform m_transform;
};

} // namespace Vulkan
