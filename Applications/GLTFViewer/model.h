#ifndef GLTFVIEWER_MODEL_H
#define GLTFVIEWER_MODEL_H

#include <slim/slim.hpp>

using namespace slim;

struct GLTFVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec4 tangent;
    glm::vec2 uv0;
    glm::vec2 uv1;
    glm::vec4 color0;
    glm::vec4 joints0;
    glm::vec4 weights0;
};

struct GLTFPrimitive {
    VkPrimitiveTopology topology;
    BoundingBox boundingBox;
    SmartPtr<Mesh> mesh;
    SmartPtr<Material> material;
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;
};

struct GLTFMesh {
    std::vector<GLTFPrimitive> primitives;
};

struct GLTFScene {
    std::string name;
    std::vector<SmartPtr<Scene>> roots;
};

struct GLTFModel {
    std::vector<GLTFScene>             scenes;
    std::vector<SmartPtr<Scene>>       nodes;
    std::vector<GLTFMesh>              meshes;
    std::vector<SmartPtr<Material>>    materials;
    std::vector<SmartPtr<Sampler>>     samplers;
    std::vector<SmartPtr<GPUImage>>    images;
};

struct alignas(128) MaterialFactors {
    glm::vec4 baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    int       baseColorTexCoord = -1;

    float     metallicFactor = 1.0f;
    float     roughnessFactor = 1.0f;
    int       metallicRoughnessTexCoord = -1;

    glm::vec3 emissiveFactor = glm::vec3(0.0f, 0.0f, 0.0f);
    int       emissiveTexCoord = -1;

    float     occlusionFactor = 1.0f;
    int       occlusionTexCoord = -1;

    float     normalTexScale = 1.0f;
    int       normalTexCoord = -1;
};

class GLTFAssetManager : public ReferenceCountable {
public:
    explicit GLTFAssetManager(Device* device);
    GLTFModel Load(CommandBuffer* commandBuffer, const std::string& path);

private:
    void LoadSamplers(GLTFModel& result, const tinygltf::Model& model);
    void LoadImages(GLTFModel& result, const tinygltf::Model& model, CommandBuffer* commandBuffer, const std::string& basedir);
    void LoadMaterials(GLTFModel& result, const tinygltf::Model& model);
    void LoadMeshes(GLTFModel& result, const tinygltf::Model& model, CommandBuffer* commandBuffer);
    void LoadNodes(GLTFModel& result, const tinygltf::Model& model);
    void LoadScenes(GLTFModel& result, const tinygltf::Model& model);

    void ReadVertexPosition(std::vector<GLTFVertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor);
    void ReadVertexNormal(std::vector<GLTFVertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor);
    void ReadVertexTangent(std::vector<GLTFVertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor);
    void ReadVertexTexCoord0(std::vector<GLTFVertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor);
    void ReadVertexTexCoord1(std::vector<GLTFVertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor);
    void ReadVertexColor0(std::vector<GLTFVertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor);
    void ReadVertexJoints0(std::vector<GLTFVertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor);
    void ReadVertexWeights0(std::vector<GLTFVertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor);
    void ReadIndices(std::vector<uint32_t>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor);

private:
    SmartPtr<Device>                  device;
    SmartPtr<SceneManager>            manager;
    SmartPtr<Shader>                  vShaderPbr;
    SmartPtr<Shader>                  fShaderPbr;
    SmartPtr<Technique>               techniqueOpaque;
    SmartPtr<Technique>               techniqueMask;
    SmartPtr<Technique>               techniqueBlend;
};

#endif // GLTFVIEWER_MODEL_H
