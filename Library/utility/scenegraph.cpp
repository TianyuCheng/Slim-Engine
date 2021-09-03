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
    });

    if (accelBuilder.get()) {
        // build blas
        for (auto& mesh : meshes) {
            accelBuilder->AddMesh(mesh);
        }
        accelBuilder->BuildBlas();

        // build transform buffer (prepare for tlas)
        device->Execute([&](CommandBuffer* commandBuffer) {
            BuildTransformBuffer(commandBuffer, bufferUsage, memoryUsage);
        });

        // build tlas
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

void scene::Builder::EnableASCompaction() {
    accelBuilder->EnableCompaction();
}

VkBufferUsageFlags scene::Builder::GetCommonBufferUsages() const {
    VkBufferUsageFlags commonBufferUsageFlags = 0;
    if (accelBuilder) {
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
    for (const auto& attrib : mesh->vertexData) {
        mesh->vertexBuffers.push_back(*mesh->vertexBuffer);
        mesh->vertexOffsets.push_back(vertexBufferOffset);
        commandBuffer->CopyDataToBuffer(attrib, mesh->vertexBuffer, vertexBufferOffset);
    }
}

void scene::Builder::BuildTransformBuffer(CommandBuffer* commandBuffer,
                                          VkBufferUsageFlags bufferUsage,
                                          VmaMemoryUsage memoryUsage) {
    uint64_t transformId = 0;
    uint64_t numTransforms = 0;
    for (auto& node : nodes) {
        numTransforms += node->NumDraws();
    }

    uint64_t bufferSize = numTransforms * sizeof(VkAccelerationStructureInstanceKHR);
    transformBuffer = SlimPtr<Buffer>(device, bufferSize, bufferUsage, memoryUsage);

    std::vector<VkAccelerationStructureInstanceKHR> instances;
    for (auto& node : nodes) {
        for (auto& [mesh, _] : *node) {
            accel::AccelStruct* as = mesh->blas->accel;
            #ifndef NDEBUG
            if (as == nullptr) {
                throw std::runtime_error("node's geometry blas has not been built!");
            }
            #endif
            instances.push_back(VkAccelerationStructureInstanceKHR { });
            auto& instance = instances.back();
            instance.transform = node->GetVkTransformMatrix();
            instance.instanceCustomIndex = transformId++;
            instance.accelerationStructureReference = device->GetDeviceAddress(as);
            instance.instanceShaderBindingTableRecordOffset = 0;                        // TODO: We will use the same hit group for all objects
            instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR; // TODO: we will hard code back face culling for now
            instance.mask = 0xff;
        }
    }

    commandBuffer->CopyDataToBuffer(instances, transformBuffer);
    accelBuilder->AddInstances(transformBuffer, 0, numTransforms);
}
