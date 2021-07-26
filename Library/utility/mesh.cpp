#include "utility/mesh.h"

using namespace slim;

void Mesh::SetVertexAttrib(Buffer *buffer, size_t offset, uint32_t index) {
    // prepare vertex buffer
    if (vertexBuffers.size() <= index) { vertexBuffers.resize(index + 1); }
    if (vertexOffsets.size() <= index) { vertexOffsets.resize(index + 1); }

    // create vertex buffer
    vertexOffsets[index] = offset;
    vertexBuffers[index] = buffer;
}

void Mesh::SetIndexAttrib(Buffer *buffer, size_t offset, VkIndexType type) {
    // prepare index buffer
    indexBuffer = buffer;
    indexOffset = offset;
    indexType = type;
}

Submesh::Submesh() {
}

Submesh::Submesh(Mesh *mesh, size_t vertexOffset, size_t indexOffset, size_t indexCount)
    : mesh(mesh), vertexOffset(vertexOffset), indexOffset(indexOffset), indexCount(indexCount) {
}

void Submesh::Bind(CommandBuffer *commandBuffer) const {
    commandBuffer->BindVertexBuffers(0, mesh->vertexBuffers, mesh->vertexOffsets);
    commandBuffer->BindIndexBuffer(mesh->indexBuffer, mesh->indexOffset, mesh->indexType);
}

void Submesh::Draw(CommandBuffer *commandBuffer) const {
    commandBuffer->DrawIndexed(indexCount, instanceCount, indexOffset, vertexOffset, 0);
}
