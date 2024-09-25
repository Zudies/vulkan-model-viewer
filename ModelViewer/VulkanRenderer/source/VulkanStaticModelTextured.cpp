#include "pch.h"
#include "VulkanStaticModelTextured.h"
#include "VulkanRendererImpl.h"

//TODO: more generic class
#include "VulkanRendererSceneImpl_Basic.h"

#include "ModelObjLoader.h"

#include "glm/gtc/matrix_inverse.hpp"
#include <filesystem>

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
        (void)meshFaceCount;

        // Combine all meshes into one
        if (meshIndex == 0) {
            AddMesh(sizeof(VulkanTexturedVertex) * vertexIndexCount);
        }
        else {
            ReserveCurrentMesh(sizeof(VulkanTexturedVertex) * vertexIndexCount);
        }

        for (uint32_t i = 0; i < vertexIndexCount; ++i) {
            VulkanTexturedVertex newVert{};

            auto index = FaceIndex(i);

            // Engine uses left handle system with axes on [+x, +y, +z] for [Right, Up, Forward]
            newVert.position.x = -AttributeVertex(index.VertexIndex * 3 + 0);
            newVert.position.y = AttributeVertex(index.VertexIndex * 3 + 2);
            newVert.position.z = -AttributeVertex(index.VertexIndex * 3 + 1);

            newVert.normal.x = -AttributeNormal(index.NormalIndex * 3 + 0);
            newVert.normal.y = AttributeNormal(index.NormalIndex * 3 + 2);
            newVert.normal.z = -AttributeNormal(index.NormalIndex * 3 + 1);

            newVert.color.r = AttributeColor(index.VertexIndex * 3 + 0);
            newVert.color.g = AttributeColor(index.VertexIndex * 3 + 1);
            newVert.color.b = AttributeColor(index.VertexIndex * 3 + 2);

            newVert.texCoord.x = AttributeTexCoord(index.TexCoordIndex * 2 + 0);
            newVert.texCoord.y = 1.0f - AttributeTexCoord(index.TexCoordIndex * 2 + 1);

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

VulkanStaticModelTextured::VulkanStaticModelTextured(RendererSceneImpl_Basic *owner)
  : m_owner(owner),
    m_vertexData(owner->GetRenderer()),
    m_descriptorSet(owner->GetRenderer()),
    m_accumulatedTime(0.0) {
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
    std::filesystem::path texturePath(objFilePath);
    texturePath.replace_extension(".png");
    auto &firstTexture = m_materialData.emplace_back(m_owner->GetRenderer());
    firstTexture.LoadImageFromFile(texturePath.u8string());
    firstTexture.FlushTextureToDevice();
    firstTexture.ClearHostResources();

    auto &firstSampler = m_samplers.emplace_back(m_owner->GetRenderer());
    firstSampler.Initialize();

    // Upload vertex and index data
    m_vertexData.SetVertexCount(loader.GetVertexCount(0));
    m_vertexData.SetIndexCount(loader.GetIndexCount(0));
    //TODO: Add buffer type that allows using the vertex in-memory instead of needing to copy it
    memcpy(m_vertexData.GetVertexData(), loader.GetVertexData(0), loader.GetVertexSize() * loader.GetVertexCount(0));
    memcpy(m_vertexData.GetIndexData(), loader.GetIndexData(0), loader.GetIndexSize() * loader.GetIndexCount(0));
    m_vertexData.FlushVertexToDevice();
    m_vertexData.FlushIndexToDevice();

    // Setup descriptor set
    VulkanDescriptorSetLayout *layout = m_owner->GetDescriptorSetLayout(RENDERABLE_OBJECT_TYPE_STATIC_MODEL_TEXTURED);
    m_descriptorSet.SetDescriptorSetLayout(layout);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = firstTexture.GetDeviceImageView();
    imageInfo.sampler = firstSampler.GetVkSampler();
    m_descriptorSet.UpdateDescriptorWrite(0, &imageInfo);

    // Descriptor set will not be changing so allocate it in persistent pool
    VulkanDescriptorSetAllocator *persistentPool = m_owner->GetPersistentDescriptorPool();
    auto err = persistentPool->AllocateDescriptorSet(1, &m_descriptorSet);
    if (err != Graphics::GraphicsError::OK) {
        return err;
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError VulkanStaticModelTextured::Draw(f64 deltaTime) {
    // Update transform
    m_accumulatedTime += deltaTime;
    //m_transform.SetRotation(0.0f, m_accumulatedTime * glm::radians(90.0f), 0.0f);

    Graphics::Camera *camera = m_owner->GetCamera();
    glm::mat4x4 modelMatrix = m_transform.GetTransformMatrix();
    glm::mat4x4 normalMatrix = glm::inverseTranspose(camera->ViewMatrix() * modelMatrix);

    VulkanPipeline *pipeline = m_owner->GetPipeline(RENDERABLE_OBJECT_TYPE_STATIC_MODEL_TEXTURED);
    VulkanCommandBuffer *commandBuffer = m_owner->GetMainCommandBuffer();

    // Bind the pipeline for this object type
    m_owner->CommandBindPipeline(commandBuffer, pipeline);

    // Bind vertex buffer data
    VkDeviceSize offsets = 0;
    vkCmdBindVertexBuffers(commandBuffer->GetVkCommandBuffer(), 0, 1, &m_vertexData.GetVertexDeviceBuffer(), &offsets);
    vkCmdBindIndexBuffer(commandBuffer->GetVkCommandBuffer(), m_vertexData.GetIndexDeviceBuffer(), 0, VK_INDEX_TYPE_UINT32);

    // Bind model matrix as a push constant
    vkCmdPushConstants(commandBuffer->GetVkCommandBuffer(), pipeline->GetVkPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4x4), &modelMatrix);
    vkCmdPushConstants(commandBuffer->GetVkCommandBuffer(), pipeline->GetVkPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4x4), sizeof(glm::mat4x4), &normalMatrix);

    // Bind descriptor sets
    VkDescriptorSet bindDescriptorSets[] = { m_descriptorSet.GetVkDescriptorSet() };
    vkCmdBindDescriptorSets(commandBuffer->GetVkCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetVkPipelineLayout(), 1, countof(bindDescriptorSets), bindDescriptorSets, 0, nullptr);

    vkCmdDrawIndexed(commandBuffer->GetVkCommandBuffer(), static_cast<uint32_t>(m_vertexData.GetIndexCount()), 1, 0, 0, 0);

    return Graphics::GraphicsError::OK;
}

} // namespace Vulkan
