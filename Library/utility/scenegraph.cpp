#include <deque>
#include "utility/scenegraph.h"

using namespace slim;

Scene::Node::Node() : name(), parent(nullptr) {
}

Scene::Node::Node(const std::string& name, Node* parent)
    : name(name) {
    MoveTo(parent);
}

void Scene::Node::AddChild(Node* child) {
    child->MoveTo(this);
}

void Scene::Node::MoveTo(Node* parent) {
    // check if parent is the same
    // skip if no movement
    if (this->parent == parent) {
        return;
    }

    // check if this node has a parent
    // remove this node from parent
    if (this->parent) {
        this->parent->children.remove(this);
    }

    // add to parent
    this->parent = parent;
    parent->children.push_back(this);
}

void Scene::Node::SetDraw(Mesh* mesh, Material* material) {
    drawables.clear();
    drawables.push_back(std::make_tuple(mesh, material));
}

void Scene::Node::AddDraw(Mesh* mesh, Material* material) {
    drawables.push_back(std::make_tuple(mesh, material));
}

// transform
void Scene::Node::Scale(float x, float y, float z) {
    transform.Scale(x, y, z);
}

void Scene::Node::Rotate(const glm::vec3& axis, float radians) {
    transform.Rotate(axis, radians);
}

void Scene::Node::Rotate(float x, float y, float z, float w) {
    transform.Rotate(x, y, z, w);
}

void Scene::Node::Translate(float x, float y, float z) {
    transform.Translate(x, y, z);
}

void Scene::Node::SetTransform(const Transform& transform) {
    this->transform = transform;
}

VkTransformMatrixKHR Scene::Node::GetVkTransformMatrix() const {
    const glm::mat4& localToWorld = transform.LocalToWorld();

    VkTransformMatrixKHR matrix = {};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            matrix.matrix[i][j] = localToWorld[i][j];
        }
    }
    return matrix;
}

void Scene::Node::ForEach(const std::function<bool(Scene::Node*)> &callback) {
    std::deque<Scene::Node*> nodes = { this };
    while (!nodes.empty()) {
        Scene::Node* node = nodes.front();
        // check callback to determine if we need to continue traversal
        if (callback(node)) {
            for (Scene::Node* child : node->children) {
                nodes.push_back(child);
            }
        }
        nodes.pop_front();
    }
}

void Scene::Node::ApplyTransform() {
    ForEach([](Scene::Node* node) {
        if (node->parent) {
            node->transform.ApplyTransform(node->parent->transform);
        } else {
            node->transform.ApplyTransform();
        }
        return true;
    });
}

Scene::Builder::Builder(Device* device) : device(device) {
}

void Scene::Builder::Build() {
    auto [vertexBufferSize, indexBufferSize] = CalculateBufferSizes();

    // prepare vertex buffer for all geometries in the scene
    vertexBuffer = SlimPtr<Buffer>(device, vertexBufferSize,
                                   commonBufferUsageFlags | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                   VMA_MEMORY_USAGE_CPU_TO_GPU);

    // prepare index buffer for all geometries in the scene
    indexBuffer = SlimPtr<Buffer>(device, indexBufferSize,
                                  commonBufferUsageFlags | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                  VMA_MEMORY_USAGE_CPU_TO_GPU);

    // buffer handles
    VkBuffer vBuffer = *vertexBuffer;
    VkBuffer iBuffer = *indexBuffer;

    // copy the data of the whole scene to gpu
    uint8_t* vertexData = vertexBuffer->GetData<uint8_t>();
    uint8_t* indexData = indexBuffer->GetData<uint8_t>();
    for (Mesh* mesh : meshes) {
        std::memcpy(vertexData + mesh->vertexOffset, mesh->vertexData.data(), mesh->vertexData.size());
        std::memcpy(indexData + mesh->indexOffset, mesh->indexData.data(), mesh->indexData.size());
        // assign the vertex and index buffers
        mesh->indexBuffer = iBuffer;
        mesh->vertexBuffers.resize(mesh->relativeOffsets.size());
        for (uint32_t i = 0; i < mesh->relativeOffsets.size(); i++) {
            mesh->vertexBuffers[i] = vBuffer;
        }
        #ifndef NDBUEG
        // mark mesh as built
        mesh->built = true;
        #endif
    }

    vertexBuffer->Flush();
    indexBuffer->Flush();
}

void Scene::Builder::Clear() {
    nodes.clear();
    meshes.clear();
}

std::tuple<uint64_t, uint64_t> Scene::Builder::CalculateBufferSizes() const {
    uint64_t vertexBufferSize = 0;
    uint64_t indexBufferSize = 0;
    for (Mesh* mesh : meshes) {

        // update vertex and index buffer offsets
        mesh->indexOffset = indexBufferSize;
        mesh->vertexOffset = vertexBufferSize;
        mesh->vertexOffsets.reserve(mesh->relativeOffsets.size());
        for (auto relativeOffset : mesh->relativeOffsets) {
            mesh->vertexOffsets.push_back(vertexBufferSize + relativeOffset);
        }

        // update total vertex and index buffer size
        vertexBufferSize += mesh->vertexData.size();
        indexBufferSize += mesh->indexData.size();
    }
    return std::make_tuple(vertexBufferSize, indexBufferSize);
}
