#pragma once
#include <unordered_map>

namespace Graphics {

class ModelObjVertexWriter;
class ModelObjAttributeFetcher;

class ModelObjLoader {
public:
    typedef std::vector<uint8_t> DataBuffer;
    typedef std::vector<DataBuffer> MeshDataBuffer;

public:
    ModelObjLoader();
    ~ModelObjLoader();

    std::string Load(std::string const &objFilePath, ModelObjVertexWriter *vertexWriter);

    uint32_t GetVertexSize() const;
    uint32_t GetVertexCount(uint32_t meshIndex) const;
    const void *GetVertexData(uint32_t meshIndex) const;

    uint32_t GetIndexSize() const;
    uint32_t GetIndexCount(uint32_t meshIndex) const;
    const void *GetIndexData(uint32_t meshIndex) const;

    //TODO: materials

private:
    uint32_t m_vertexSize;
    uint32_t m_indexSize;
    MeshDataBuffer m_vertexData;
    MeshDataBuffer m_indexData;
};

class ModelObjVertexWriter {
protected:
    struct IndexType {
        // 0xFFFFFFFF means unused
        uint32_t VertexIndex;
        uint32_t NormalIndex;
        uint32_t TexCoordIndex;
    };

public:
    ModelObjVertexWriter();
    ModelObjVertexWriter(ModelObjVertexWriter const &) = delete;
    ModelObjVertexWriter &operator=(ModelObjVertexWriter const &) = delete;
    virtual ~ModelObjVertexWriter();

    // Required override
    // Returns the size in bytes of a single vertex
    virtual uint32_t GetVertexSize() = 0;

    // Returns the size in bytes of a single index
    // Default is 4 (uint32_t)
    virtual uint32_t GetIndexSize();

    // Required override
    // This function is responsible for building all vertices
    // This will be called once per mesh face
    virtual void WriteMesh(uint32_t meshIndex, uint32_t meshFaceCount, uint32_t vertexIndexCount) = 0;

    // Starts a new mesh, returning the index of the new mesh
    virtual uint32_t AddMesh(uint32_t estimatedDataSize);

    // Attempts to reserve additional memory in the current mesh
    virtual void ReserveCurrentMesh(uint32_t estimatedDataSize);

    // Adds a vertex to the last created mesh, returning the index of the (new) vertex
    virtual uint32_t AddVertex(void *vertexData);

protected:
    // Fetches the obj file's attribute data
    f32 AttributeVertex(uint32_t i);       // Corresponds to VertexIndex
    f32 AttributeVertexWeight(uint32_t i); // Corresponds to VertexIndex / 3 (Per-triangle)
    f32 AttributeNormal(uint32_t i);       // Corresponds to NormalIndex
    f32 AttributeTexCoord(uint32_t i);     // Corresponds to TexCoordIndex
    f32 AttributeTexCoordW(uint32_t i);    // w component of TexCoords, if there is one
    f32 AttributeColor(uint32_t i);        // Corresponds to VertexIndex
    //TODO: skin weight

    // Fetches the current mesh's index data
    IndexType FaceIndex(uint32_t i);           // One per vertex
    int FaceMaterialId(uint32_t i);            // One per triangle
    uint32_t FaceSmoothingGroupId(uint32_t i); // One per triangle (0 = unused)

protected:
    friend class ModelObjLoader;
    ModelObjLoader::MeshDataBuffer *m_boundVertexData;
    ModelObjLoader::MeshDataBuffer *m_boundIndexData;
    ModelObjAttributeFetcher *m_attributeFetcher;

    std::unordered_map<size_t, uint32_t> m_uniqueVertices;
};

} // namespace Graphics
