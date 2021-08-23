#include "utility/mesh.h"

using namespace slim;

void Mesh::SetVertexAttrib(Buffer *buffer, size_t offset, uint32_t index) {
    // prepare vertex buffer
    if (vertexBuffers.size() <= index) { vertexBuffers.resize(index + 1); }
    if (vertexOffsets.size() <= index) { vertexOffsets.resize(index + 1); }
    if (_vertexBuffers.size() <= index) { _vertexBuffers.resize(index + 1); }

    // create vertex buffer
    vertexOffsets[index] = offset;
    vertexBuffers[index] = buffer;

    // automatic buffer pointer management
    _vertexBuffers[index] = buffer;
}

void Mesh::SetIndexAttrib(Buffer *buffer, size_t offset, VkIndexType type) {
    // prepare index buffer
    indexBuffer = buffer;
    indexOffset = offset;
    indexType = type;

    // automatic buffer pointer management
    _indexBuffer = buffer;
}

void Mesh::Bind(CommandBuffer* commandBuffer) const {
    // bind vertex buffers
    commandBuffer->BindVertexBuffers(0, vertexBuffers, vertexOffsets);

    // bind index buffer
    if (indexBuffer) {
        commandBuffer->BindIndexBuffer(indexBuffer, indexOffset, indexType);
    }
}
