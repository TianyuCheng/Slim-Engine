#include <vector>
#include <slim/slim.hpp>

using namespace slim;

struct GLTFVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord0;
};

struct GLTFMesh {
    std::vector<SmartPtr<Mesh>> primitives;
    std::vector<Submesh>        submeshes;
    std::vector<uint32_t>       materials;
};

struct GLTFModel {
    SceneNode* root = nullptr;
    SceneGraph* defaultScene = nullptr;
    std::vector<GLTFMesh>             meshes;
    std::vector<SmartPtr<SceneNode>>  nodes;
    std::vector<SmartPtr<SceneGraph>> scenes;
    std::vector<SmartPtr<Sampler>>    samplers;
    std::vector<SmartPtr<Buffer>>     buffers;
    std::vector<SmartPtr<GPUImage2D>> images;
    std::vector<SmartPtr<Material>>   materials;

    SmartPtr<Shader> vShader;
    SmartPtr<Shader> fShader;
    SmartPtr<Technique> technique;
};

void LoadModel(Context *context, GLTFModel &model, const std::string &path);
