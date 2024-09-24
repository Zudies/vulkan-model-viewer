#include "pch.h"
#include "VulkanStaticModelTextured.h"

#include "ModelObjLoader.h"

namespace Vulkan {

bool operator==(VulkanTexturedVertex const &lhs, VulkanTexturedVertex const &rhs) {
    return lhs.position == rhs.position &&
        lhs.normal == rhs.normal &&
        lhs.color == rhs.color &&
        lhs.texCoord == rhs.texCoord;
}

// Vertex writer for this vertex type
class TexturedVertexWriter : public Graphics::ModelObjVertexWriter {
public:
    virtual uint32_t GetVertexSize() override {
        return sizeof(VulkanTexturedVertex);
    }

    virtual void WriteMesh(uint32_t meshIndex, uint32_t meshFaceCount, uint32_t vertexIndexCount) override {
        if (meshIndex == 0) {
            AddMesh(sizeof(VulkanTexturedVertex) * meshFaceCount);
        }
        else {
            ReserveCurrentMesh(sizeof(VulkanTexturedVertex) * meshFaceCount);
        }

        for (uint32_t i = 0; i < vertexIndexCount; ++i) {
            VulkanTexturedVertex newVert{};

            auto index = FaceIndex(i);
            newVert.position.x = AttributeVertex(index.VertexIndex * 3 + 0);
            newVert.position.y = AttributeVertex(index.VertexIndex * 3 + 1);
            newVert.position.z = AttributeVertex(index.VertexIndex * 3 + 2);

            newVert.normal.x = AttributeNormal(index.NormalIndex * 3 + 0);
            newVert.normal.y = AttributeNormal(index.NormalIndex * 3 + 1);
            newVert.normal.z = AttributeNormal(index.NormalIndex * 3 + 2);

            newVert.color.r = AttributeColor(index.VertexIndex * 3 + 0);
            newVert.color.g = AttributeColor(index.VertexIndex * 3 + 1);
            newVert.color.b = AttributeColor(index.VertexIndex * 3 + 2);

            newVert.texCoord.x = AttributeTexCoord(index.TexCoordIndex * 2 + 0);
            newVert.texCoord.y = AttributeTexCoord(index.TexCoordIndex * 2 + 1);

            AddVertex(&newVert);
        }
    }

    virtual bool CheckForUniqueVertex() const override {
        return true;
    }

    virtual size_t HashVertex(void *vertexData) const override {
        return std::hash<std::string_view>{}({ reinterpret_cast<char*>(vertexData), sizeof(VulkanTexturedVertex) });
    }

    virtual bool CompareVertex(void *vertexDataLeft, void *vertexDataRight) const override {
        VulkanTexturedVertex *lhs = reinterpret_cast<VulkanTexturedVertex*>(vertexDataLeft);
        VulkanTexturedVertex *rhs = reinterpret_cast<VulkanTexturedVertex*>(vertexDataRight);

        return lhs->position == rhs->position &&
            lhs->normal == rhs->normal &&
            lhs->color == rhs->color &&
            lhs->texCoord == rhs->texCoord;
    }
};

VkVertexInputBindingDescription VulkanTexturedVertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};

    bindingDescription.binding = 0;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    bindingDescription.stride = sizeof(VulkanTexturedVertex);

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 4> VulkanTexturedVertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(VulkanTexturedVertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(VulkanTexturedVertex, normal);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(VulkanTexturedVertex, color);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(VulkanTexturedVertex, texCoord);
    return attributeDescriptions;
}

VulkanStaticModelTextured::VulkanStaticModelTextured(RendererImpl *renderer)
  : m_vertexData(renderer),
    m_descriptorSetLayout(renderer),
    m_descriptorSet(renderer) {
}

VulkanStaticModelTextured::~VulkanStaticModelTextured() {
}

Graphics::GraphicsError VulkanStaticModelTextured::LoadFromObjFile(std::string const &objFilePath) {
    Graphics::ModelObjLoader loader;
    TexturedVertexWriter vertexWriter;

    auto errorString = loader.Load(objFilePath, &vertexWriter);
    if (!errorString.empty()) {
        LOG_ERROR("Error when loading obj file: %s\n%s\n", objFilePath.c_str(), errorString.c_str());
        return Graphics::GraphicsError::FILE_LOAD_ERROR;
    }

    //TODO: This should be loaded in obj file but for now we're assuming one texture and manually load it

    return Graphics::GraphicsError::OK;
}

} // namespace Vulkan
