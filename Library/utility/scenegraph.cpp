#include <deque>
#include "utility/scenegraph.h"

using namespace slim;

scene::Node::Node(Node* parent) : name(), parent(parent) {
    MoveTo(parent);
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
        for (int j = 0; j < 4; j++) {
            matrix.matrix[i][j] = localToWorld[j][i];
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

void scene::Builder::Build() {
    // prepare common buffer and memory usage flags
    VkBufferUsageFlags bufferUsage = GetCommonBufferUsages();
    VmaMemoryUsage memoryUsage = GetCommonMemoryUsages();

    // build mesh data
    device->Execute([&](CommandBuffer* commandBuffer) {
        for (auto& mesh : meshes) {
            BuildVertexBuffer(commandBuffer, mesh, bufferUsage, memoryUsage);
            BuildIndexBuffer(commandBuffer, mesh, bufferUsage, memoryUsage);
            #ifndef NDEBUG
            mesh->built = true;
            #endif
        }
        BuildAabbsBuffer(commandBuffer, bufferUsage, memoryUsage);
    });

    // build acceleration structure if needed to
    if (accelBuilder.get()) {
        uint32_t aabbsGeometryIndex = 0;

        // add mesh-based blas
        for (auto& mesh : meshes) {
            accelBuilder->AddMesh(mesh);
        }
        // add aabb-based blas
        if (aabbsBuffer) {
            aabbsGeometryIndex = accelBuilder->AddAABBs(aabbsBuffer, aabbs.size(), sizeof(VkAabbPositionsKHR));
        }
        // build blas
        accelBuilder->BuildBlas();

        // add mesh-based tlas
        for (auto& node : nodes) {
            accelBuilder->AddNode(node, 0, 0x1);       // use sbt record 0 (for triangle mesh)
        }
        // add aabb-based tlas
        if (aabbsBuffer) {
            // update blas geometry
            aabbsMesh->SetBlasGeometry(accelBuilder->GetBlasGeometry(aabbsGeometryIndex));
            accelBuilder->AddNode(aabbsNode, 1, 0x2);  // use sbt record 1 (for procedural)
        }
        accelBuilder->BuildTlas();
    }
}

void scene::Builder::Clear() {
    nodes.clear();
    meshes.clear();
}

void scene::Builder::EnableRayTracing() {
    accelBuilder = SlimPtr<accel::Builder>(device);
}

void scene::Builder::AddAABB(const BoundingBox& aaBox) {
    AddAABBs(aaBox, 1);
}

void scene::Builder::AddAABBs(const BoundingBox& aaBox, uint32_t count) {
    #ifndef NDEBUG
    if (!accelBuilder.get()) {
        throw std::runtime_error("scene::Builder::AddAABBs() is only supported when accel builder is enabled!");
    }
    #endif

    const auto& min = aaBox.Min();
    const auto& max = aaBox.Max();

    VkAabbPositionsKHR aabb;
    aabb.minX = min.x;
    aabb.minY = min.y;
    aabb.minZ = min.z;
    aabb.maxX = max.x;
    aabb.maxY = max.y;
    aabb.maxZ = max.z;

    if (count <= 1) {
        aabbs.push_back(aabb);
    } else {
        aabbs.reserve(aabbs.size() + count);
        for (uint32_t i = 0; i < count; i++) {
            aabbs.push_back(aabb);
        }
    }
}

VkBufferUsageFlags scene::Builder::GetCommonBufferUsages() const {
    VkBufferUsageFlags commonBufferUsageFlags = 0;
    if (accelBuilder) {
        commonBufferUsageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        commonBufferUsageFlags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        commonBufferUsageFlags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    }
    return commonBufferUsageFlags;
}

VmaMemoryUsage scene::Builder::GetCommonMemoryUsages() const {
    return VMA_MEMORY_USAGE_GPU_ONLY;
}

void scene::Builder::BuildIndexBuffer(CommandBuffer* commandBuffer, Mesh* mesh, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage) {
    // create and update index buffer
    if (mesh->indexCount > 0) {
        mesh->indexOffset = 0;
        mesh->indexBuffer = SlimPtr<Buffer>(device, mesh->indexData.size(),
                                            bufferUsage | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                            memoryUsage);
        mesh->indexBuffer->SetName("Scene Mesh Index Buffer");
        commandBuffer->CopyDataToBuffer(mesh->indexData, mesh->indexBuffer, mesh->indexOffset);
    }
}

void scene::Builder::BuildVertexBuffer(CommandBuffer* commandBuffer, Mesh* mesh, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage) {
    // create and update vertex buffer
    uint64_t vertexBufferSize = 0;
    uint64_t vertexBufferOffset = 0;
    for (const auto& attrib : mesh->vertexData) {
        vertexBufferSize += attrib.size();
    }
    mesh->vertexBuffer = SlimPtr<Buffer>(device, vertexBufferSize,
                                         bufferUsage | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                         memoryUsage);
    mesh->vertexBuffer->SetName("Scene Mesh Vertex Buffer");
    for (const auto& attrib : mesh->vertexData) {
        mesh->vertexBuffers.push_back(*mesh->vertexBuffer);
        mesh->vertexOffsets.push_back(vertexBufferOffset);
        commandBuffer->CopyDataToBuffer(attrib, mesh->vertexBuffer, vertexBufferOffset);
    }
}

void scene::Builder::BuildAabbsBuffer(CommandBuffer* commandBuffer,
                                      VkBufferUsageFlags bufferUsage,
                                      VmaMemoryUsage memoryUsage) {
    // prepare aabbs buffer
    uint64_t aabbsBufferSize = aabbs.size() * sizeof(VkAabbPositionsKHR);
    if (aabbsBufferSize == 0) return;

    // allow compute shader to use it as a storage buffer
    bufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    // create aabbs buffer and aabbs node
    aabbsNode = SlimPtr<Node>();
    aabbsMesh = SlimPtr<Mesh>();
    aabbsNode->SetDraw(aabbsMesh, nullptr);
    aabbsBuffer = SlimPtr<Buffer>(device, aabbsBufferSize, bufferUsage, memoryUsage);
    aabbsBuffer->SetName("Scene AABBs Buffer");

    // copy bounding boxes to dest type
    std::vector<VkAabbPositionsKHR> data(aabbs.size());
    commandBuffer->CopyDataToBuffer(aabbs, aabbsBuffer);
}
