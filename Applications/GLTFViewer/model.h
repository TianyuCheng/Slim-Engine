#include <vector>
#include <slim/slim.hpp>

using namespace slim;

struct GltfModel {
    SceneNode* root = nullptr;
    std::vector<SmartPtr<Mesh>> meshes;
    std::vector<SmartPtr<SceneNode>> nodes;
    std::vector<SmartPtr<GPUImage2D>> images;
};

void LoadModel(Context *context, GltfModel &model, const std::string &path);
