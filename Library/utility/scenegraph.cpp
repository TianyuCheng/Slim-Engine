#include <deque>
#include "utility/scenegraph.h"

using namespace slim;

scene::Node::Node() : name(), parent(nullptr) {
}

scene::Node::Node(const std::string& name, Node* parent)
    : name(name) {
    MoveTo(parent);
}

void scene::Node::AddChild(Node* child) {
    child->MoveTo(this);
}

void scene::Node::MoveTo(Node* parent) {
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

void scene::Node::SetDraw(Mesh* mesh, Material* material) {
    drawables.clear();
    drawables.push_back(std::make_tuple(mesh, material));
}

void scene::Node::AddDraw(Mesh* mesh, Material* material) {
    drawables.push_back(std::make_tuple(mesh, material));
}

// transform
void scene::Node::Scale(float x, float y, float z) {
    transform.Scale(x, y, z);
}

void scene::Node::Rotate(const glm::vec3& axis, float radians) {
    transform.Rotate(axis, radians);
}

void scene::Node::Rotate(float x, float y, float z, float w) {
    transform.Rotate(x, y, z, w);
}

void scene::Node::Translate(float x, float y, float z) {
    transform.Translate(x, y, z);
}

void scene::Node::SetTransform(const Transform& transform) {
    this->transform = transform;
}

VkTransformMatrixKHR scene::Node::GetVkTransformMatrix() const {
    const glm::mat4& localToWorld = transform.LocalToWorld();

    VkTransformMatrixKHR matrix = {};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            matrix.matrix[i][j] = localToWorld[i][j];
        }
    }
    return matrix;
}

void scene::Node::ForEach(const std::function<bool(scene::Node*)> &callback) {
    std::deque<scene::Node*> nodes = { this };
    while (!nodes.empty()) {
        scene::Node* node = nodes.front();
        // check callback to determine if we need to continue traversal
        if (callback(node)) {
            for (scene::Node* child : node->children) {
                nodes.push_back(child);
            }
        }
        nodes.pop_front();
    }
}

void scene::Node::ApplyTransform() {
    ForEach([](scene::Node* node) {
        if (node->parent) {
            node->transform.ApplyTransform(node->parent->transform);
        } else {
            node->transform.ApplyTransform();
        }
        return true;
    });
}

scene::Builder::Builder(Device* device) : device(device) {
}

void scene::Builder::Build(CommandBuffer* commandBuffer) {
    auto [vertexBufferSize, indexBufferSize] = CalculateBufferSizes();

    VkBufferUsageFlags commonBufferUsageFlags = 0;
    if (enableRayTracing) {
        commonBufferUsageFlags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        commonBufferUsageFlags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    }

    // prepare vertex buffer for all geometries in the scene
    vertexBuffer = SlimPtr<Buffer>(device, vertexBufferSize,
                                   commonBufferUsageFlags | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                   VMA_MEMORY_USAGE_GPU_ONLY);

    // prepare index buffer for all geometries in the scene
    indexBuffer = SlimPtr<Buffer>(device, indexBufferSize,
                                  commonBufferUsageFlags | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                  VMA_MEMORY_USAGE_GPU_ONLY);

    // buffer handles
    VkBuffer vBuffer = *vertexBuffer;
    VkBuffer iBuffer = *indexBuffer;

    // copy the data of the whole scene to gpu
    std::vector<uint8_t> vertexData(vertexBufferSize);
    std::vector<uint8_t> indexData(indexBufferSize);
    for (Mesh* mesh : meshes) {
        std::memcpy(vertexData.data() + mesh->vertexOffset, mesh->vertexData.data(), mesh->vertexData.size());
        std::memcpy(indexData.data() + mesh->indexOffset, mesh->indexData.data(), mesh->indexData.size());
        // assign the vertex and index buffers
        mesh->indexBuffer = iBuffer;
        mesh->vertexBuffers.resize(mesh->relativeOffsets.size());
        for (uint32_t i = 0; i < mesh->relativeOffsets.size(); i++) {
            mesh->vertexBuffers[i] = vBuffer;
        }
        // keep the original vertex/index buffer handle for other usage
        mesh->iBuffer = indexBuffer;
        mesh->vBuffer = vertexBuffer;
        #ifndef NDBUEG
        // mark mesh as built
        mesh->built = true;
        #endif
    }
    commandBuffer->CopyDataToBuffer(vertexData, vertexBuffer);
    commandBuffer->CopyDataToBuffer(indexData, indexBuffer);
}

void scene::Builder::Clear() {
    nodes.clear();
    meshes.clear();
}

void scene::Builder::EnableRayTracing() {
    enableRayTracing = true;
}

std::tuple<uint64_t, uint64_t> scene::Builder::CalculateBufferSizes() const {
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
