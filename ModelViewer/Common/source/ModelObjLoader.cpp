#include "pch.h"
#include "ModelObjLoader.h"
#include <filesystem>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

namespace Graphics {

// Local implementation of ModelObjAttributeFetcher for fetching the stored attribute data of loaded obj files
class ModelObjAttributeFetcher {
public:
    ModelObjAttributeFetcher() 
      : Vertices(nullptr),
        VertexWeights(nullptr),
        Normals(nullptr),
        TexCoords(nullptr),
        TexCoordsW(nullptr),
        Colors(nullptr),
        Indices(nullptr),
        MaterialIds(nullptr),
        SmoothingGroupIds(nullptr) {
    }

    ~ModelObjAttributeFetcher() {
    }

    // Attribute Data
    std::vector<float> *Vertices;
    std::vector<float> *VertexWeights;
    std::vector<float> *Normals;
    std::vector<float> *TexCoords;
    std::vector<float> *TexCoordsW;
    std::vector<float> *Colors;
    //TODO: skin weights

    // Mesh Data
    std::vector<tinyobj::index_t> *Indices;
    std::vector<int> *MaterialIds;
    std::vector<unsigned int> *SmoothingGroupIds;
};

ModelObjLoader::ModelObjLoader()
  : m_vertexSize(0),
    m_indexSize(sizeof(uint32_t)) {
}

ModelObjLoader::~ModelObjLoader() {

}
#pragma warning( push )
#pragma warning( disable : 6011 )
std::string ModelObjLoader::Load(std::string const &objFilePath, ModelObjVertexWriter *vertexWriter) {
    ASSERT(vertexWriter);

#ifdef _WIN32
    wchar_t cwd[MAX_PATH];
    auto ret = GetModuleFileName(NULL, cwd, MAX_PATH);
    ASSERT(ret != 0);
    std::filesystem::path exePath(cwd);
    exePath = exePath.parent_path();
#else
#error Not Supported
#endif

    exePath /= objFilePath;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, exePath.u8string().c_str())) {
        return warn + err;
    }

    /* Initialize the vertex writer */
    ModelObjAttributeFetcher tempAttribFetcher;
    vertexWriter->m_attributeFetcher = &tempAttribFetcher;

    // Vertex data
    m_vertexSize = vertexWriter->GetVertexSize();
    m_indexSize = vertexWriter->GetIndexSize();
    m_vertexData.clear();
    m_vertexData.reserve(shapes.size());
    m_indexData.clear();
    m_indexData.reserve(shapes.size());
    vertexWriter->m_boundVertexData = &m_vertexData;
    vertexWriter->m_boundIndexData = &m_indexData;

    // Attribute data
    tempAttribFetcher.Vertices = &attrib.vertices;
    tempAttribFetcher.VertexWeights = &attrib.vertex_weights;
    tempAttribFetcher.Normals = &attrib.normals;
    tempAttribFetcher.TexCoords = &attrib.texcoords;
    tempAttribFetcher.TexCoordsW = &attrib.texcoord_ws;
    tempAttribFetcher.Colors = &attrib.colors;
    //TODO: skin weight

    /* Call the vertex writer to build each mesh */
    for (size_t i = 0; i < shapes.size(); ++i) {
        // Index data
        tempAttribFetcher.Indices = &shapes[i].mesh.indices;
        tempAttribFetcher.MaterialIds = &shapes[i].mesh.material_ids;
        tempAttribFetcher.SmoothingGroupIds = &shapes[i].mesh.smoothing_group_ids;

        vertexWriter->WriteMesh(
            static_cast<uint32_t>(i),
            static_cast<uint32_t>(shapes[i].mesh.indices.size() / 3),
            static_cast<uint32_t>(shapes[i].mesh.indices.size())
        );
    }

    // Try to clear up excess unused memory
    m_vertexData.shrink_to_fit();
    m_indexData.shrink_to_fit();
    for (size_t i = 0; i < m_vertexData.size(); ++i) {
        m_vertexData[i].shrink_to_fit();
        m_indexData[i].shrink_to_fit();
    }

    return "";
}
#pragma warning( pop )

uint32_t ModelObjLoader::GetVertexSize() const {
    return m_vertexSize;
}

uint32_t ModelObjLoader::GetVertexCount(uint32_t meshIndex) const {
    return static_cast<uint32_t>(m_vertexData[meshIndex].size() / m_vertexSize);
}

const void *ModelObjLoader::GetVertexData(uint32_t meshIndex) const {
    return m_vertexData[meshIndex].data();
}

uint32_t ModelObjLoader::GetIndexSize() const {
    return m_indexSize;
}

uint32_t ModelObjLoader::GetIndexCount(uint32_t meshIndex) const {
    return static_cast<uint32_t>(m_indexData[meshIndex].size() / m_indexSize);
}

const void *ModelObjLoader::GetIndexData(uint32_t meshIndex) const {
    return m_indexData[meshIndex].data();
}

ModelObjVertexWriter::ModelObjVertexWriter()
  : m_boundVertexData(nullptr),
    m_boundIndexData(nullptr),
    m_attributeFetcher(nullptr) {
}

ModelObjVertexWriter::~ModelObjVertexWriter() {
}

uint32_t ModelObjVertexWriter::GetIndexSize() {
    return sizeof(uint32_t);
}

uint32_t ModelObjVertexWriter::AddMesh(uint32_t estimatedDataSize) {
    uint32_t newIndex = static_cast<uint32_t>(m_boundVertexData->size());

    auto &newMeshVertex = m_boundVertexData->emplace_back();
    auto &newMeshIndex = m_boundIndexData->emplace_back();

    newMeshVertex.reserve(estimatedDataSize);
    newMeshIndex.reserve((estimatedDataSize / GetVertexSize()) * GetIndexSize());

    m_uniqueVertices.clear();

    return newIndex;
}

void ModelObjVertexWriter::ReserveCurrentMesh(uint32_t estimatedDataSize) {
    m_boundVertexData->back().reserve(m_boundVertexData->back().size() + estimatedDataSize);
    m_boundIndexData->back().reserve((m_boundIndexData->back().size() + estimatedDataSize / GetVertexSize()) * GetIndexSize());
}

uint32_t ModelObjVertexWriter::AddVertex(void *vertexData) {
    uint32_t vertexSize = GetVertexSize();
    uint32_t indexSize = GetIndexSize();
    ModelObjLoader::DataBuffer &vertexBuffer = m_boundVertexData->back();
    ModelObjLoader::DataBuffer &indexBuffer = m_boundIndexData->back();

    // Do a simple hash on the data
    // TODO: This can technically result in collisions, causing vertices to be missing
    //       Better solution is to have a templated Vertex type to hash on
    size_t hashValue = std::hash<std::string_view>{}({ reinterpret_cast<char*>(vertexData), vertexSize });

    // Check if this is a new vertex or if it already exists in the buffer somewhere
    auto foundUniqueVertexIt = m_uniqueVertices.find(hashValue);
    if (foundUniqueVertexIt == m_uniqueVertices.end()) {
        uint32_t newIndex = static_cast<uint32_t>(vertexBuffer.size() / vertexSize);
        foundUniqueVertexIt = m_uniqueVertices.insert(foundUniqueVertexIt, std::make_pair(hashValue, static_cast<uint32_t>(newIndex)));
        vertexBuffer.insert(vertexBuffer.end(), vertexSize, 0);
        memcpy(vertexBuffer.data() + (newIndex * vertexSize), vertexData, vertexSize);
    }

    uint32_t newIndexBufferIndex = static_cast<uint32_t>(indexBuffer.size() / indexSize);
    indexBuffer.insert(indexBuffer.end(), indexSize, 0);
    switch (indexSize) {
    case 1:
        *reinterpret_cast<uint8_t*>(indexBuffer.data() + (newIndexBufferIndex * indexSize)) = static_cast<uint8_t>(foundUniqueVertexIt->second);
        break;
    case 2:
        *reinterpret_cast<uint16_t*>(indexBuffer.data() + (newIndexBufferIndex * indexSize)) = static_cast<uint16_t>(foundUniqueVertexIt->second);
        break;
    case 4:
        *reinterpret_cast<uint32_t*>(indexBuffer.data() + (newIndexBufferIndex * indexSize)) = foundUniqueVertexIt->second;
        break;
    case 8:
        *reinterpret_cast<uint64_t*>(indexBuffer.data() + (newIndexBufferIndex * indexSize)) = foundUniqueVertexIt->second;
        break;
    default:
        ERROR_MSG(L"Unsupported index buffer type\n");
    }

    return foundUniqueVertexIt->second;
}

f32 ModelObjVertexWriter::AttributeVertex(uint32_t i) {
    return m_attributeFetcher->Vertices->at(i);
}

f32 ModelObjVertexWriter::AttributeVertexWeight(uint32_t i) {
    return m_attributeFetcher->VertexWeights->at(i);
}

f32 ModelObjVertexWriter::AttributeNormal(uint32_t i) {
    return m_attributeFetcher->Normals->at(i);
}

f32 ModelObjVertexWriter::AttributeTexCoord(uint32_t i) {
    return m_attributeFetcher->TexCoords->at(i);
}

f32 ModelObjVertexWriter::AttributeTexCoordW(uint32_t i) {
    return m_attributeFetcher->TexCoordsW->at(i);
}

f32 ModelObjVertexWriter::AttributeColor(uint32_t i) {
    return m_attributeFetcher->Colors->at(i);
}

ModelObjVertexWriter::IndexType ModelObjVertexWriter::FaceIndex(uint32_t i) {
    IndexType ret;
    auto &implIndex = m_attributeFetcher->Indices->at(i);
    ret.VertexIndex = static_cast<uint32_t>(implIndex.vertex_index);
    ret.NormalIndex = static_cast<uint32_t>(implIndex.normal_index);
    ret.TexCoordIndex = static_cast<uint32_t>(implIndex.texcoord_index);
    return ret;
}

int ModelObjVertexWriter::FaceMaterialId(uint32_t i) {
    return m_attributeFetcher->MaterialIds->at(i);
}

uint32_t ModelObjVertexWriter::FaceSmoothingGroupId(uint32_t i) {
    return m_attributeFetcher->SmoothingGroupIds->at(i);
}

} // namespace Graphics
