#include "model.h"
#include "ghc/filesystem.hpp"

void PrintNodeHierarchy(SceneNode* node, int level = 0) {
    for (int i = 0; i < 2 * level; i++) std::cout << " ";
    std::cout << "Node: " << node->GetName() << std::endl;
    for (SceneNode* child : *node) {
        PrintNodeHierarchy(child, level + 1);
    }
}

void LoadModel(Context *context, GltfModel &result, const std::string &path) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path, tinygltf::REQUIRE_ALL);

    if (!warn.empty()) { std::cout << "Warn: " << warn << std::endl; }
    if (!err.empty()) { std::cout << "Err: " << err << std::endl; }
    if (!ret) { std::cout << "Failed to parse glTF" << std::endl; exit(1); }

    std::cout << "accessors: " << model.accessors.size() << std::endl;
    std::cout << "animations: " << model.animations.size() << std::endl;
    std::cout << "buffer: " << model.buffers.size() << std::endl;
    std::cout << "nodes: " << model.nodes.size() << std::endl;
    std::cout << "textures: " << model.textures.size() << std::endl;
    std::cout << "images: " << model.images.size() << std::endl;
    std::cout << "skin: " << model.skins.size() << std::endl;
    std::cout << "samplers: " << model.samplers.size() << std::endl;
    std::cout << "cameras: " << model.cameras.size() << std::endl;
    std::cout << "scenes: " << model.scenes.size() << std::endl;
    std::cout << "lights: " << model.lights.size() << std::endl;
    std::cout << "default scene: " << model.defaultScene << std::endl;

    // initialize scene nodes
    for (const auto &node : model.nodes) {
        // initialize scene node
        result.nodes.push_back(new SceneNode(node.name));
        SceneNode* snode = result.nodes.back();

        // initialize scene node transform
        if (node.matrix.size()) {
            glm::mat4 matrix;
            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++)
                    matrix[i][j] = node.matrix[i * 4 + j];
            snode->SetTransform(matrix);
        }
    }

    // initialize scene node hierarchy
    for (uint32_t i = 0; i < model.nodes.size(); i++) {
        SceneNode* snode = result.nodes[i];
        for (int child : model.nodes[i].children) {
            snode->AddChild(result.nodes[child]);
        }
    }

    // initialize meshes
    for (const auto &mesh : model.meshes) {
        std::cout << "mesh: " << mesh.name << std::endl;
        for (const auto &primitive : mesh.primitives) {
            for (const auto &kv : primitive.attributes) {
                std::cout << " - " << kv.first << " -> Accessors[" << kv.second << "]" << std::endl;
            }
        }
    }

#if 0
    // initialize textures
    GltfModel* pResult = &result;
    std::string basedir = ghc::filesystem::path(path).parent_path();
    context->Execute([=](CommandBuffer *commandBuffer) {
        for (const auto &image : model.images) {
            std::string filepath = basedir + "/" + image.uri;
            std::cout << "[load image] " << filepath << std::endl;
            pResult->images.push_back(TextureLoader::Load2D(commandBuffer, filepath));
        }
    });
#endif

    // SceneNode* root = result.nodes[model.defaultScene];
    // PrintNodeHierarchy(root);

    context->WaitIdle();
}
