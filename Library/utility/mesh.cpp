#include "utility/mesh.h"

using namespace slim;

Submesh::Submesh() {
}

Submesh::Submesh(Mesh *mesh, size_t vertexOffset, size_t indexOffset, size_t indexCount)
    : mesh(mesh), vertexOffset(vertexOffset), indexOffset(indexOffset), indexCount(indexCount) {
}

void Submesh::Bind(CommandBuffer *commandBuffer) const {
    std::vector<uint64_t> vertexBufferOffsets(mesh->vertexElemSizes.size());
    std::vector<VertexBuffer*> vertexBufferObjects(mesh->vertexElemSizes.size());
    for (uint32_t i = 0; i < vertexBufferOffsets.size(); i++) {
        vertexBufferOffsets[i] = mesh->vertexElemSizes[i] * vertexOffset;
        vertexBufferObjects[i] = mesh->vertexBuffers[i];
    }
    uint64_t indexBufferOffset = mesh->indexElemSize * indexOffset;

    commandBuffer->BindVertexBuffers(0, vertexBufferObjects, vertexBufferOffsets);
    commandBuffer->BindIndexBuffer(mesh->indexBuffer, indexBufferOffset);
}

void Submesh::Draw(CommandBuffer *commandBuffer) const {
    commandBuffer->DrawIndexed(indexCount, instanceCount, indexOffset, vertexOffset, 0);
}
