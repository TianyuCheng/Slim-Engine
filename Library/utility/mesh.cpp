#include "utility/mesh.h"

using namespace slim::scene;

VkAabbPositionsKHR Mesh::GetAabbPositions() const {
    const glm::vec3& min = aabb.Min();
    const glm::vec3& max = aabb.Max();
    VkAabbPositionsKHR aabb = {};
    aabb.minX = min.x;
    aabb.minY = min.y;
    aabb.minZ = min.z;
    aabb.maxX = max.x;
    aabb.maxY = max.y;
    aabb.maxZ = max.z;
    return aabb;
}

void Mesh::Bind(CommandBuffer* commandBuffer) const {
    #ifndef NDEBUG
    if (!built) {
        throw std::runtime_error("Mesh has not been built!");
    }
    if (!hasVertexAttribs) {
        throw std::runtime_error("Vertex layout has not been specified for mesh!");
    }
    #endif

    Device* device = commandBuffer->GetDevice();

    // NOTE: ditch additional library wrappings, use raw vulkan directly
    DeviceDispatch(vkCmdBindVertexBuffers(*commandBuffer, 0, vertexBuffers.size(), vertexBuffers.data(), vertexOffsets.data()));
    if (indexCount > 0) {
        DeviceDispatch(vkCmdBindIndexBuffer(*commandBuffer, *indexBuffer, indexOffset, indexType));
    }
}
