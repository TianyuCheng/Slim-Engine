#ifndef SLIM_UTILITY_MESH_H
#define SLIM_UTILITY_MESH_H

#include "core/buffer.h"
#include "core/commands.h"
#include "utility/interface.h"

namespace slim {

    class Mesh final : public NotCopyable, public NotMovable, public ReferenceCountable {
        friend struct Submesh;
    public:

        void SetVertexAttrib(Buffer *buffer, size_t offset, uint32_t index);

        void SetIndexAttrib(Buffer *buffer, size_t offset, VkIndexType indexType);

        template <typename T>
        void SetVertexAttrib(CommandBuffer *commandBuffer, const std::vector<T> &attrib, uint32_t index);

        template <typename T>
        void SetIndexAttrib(CommandBuffer *commandBuffer, const std::vector<T> &attrib);

        void Bind(CommandBuffer* commandBuffer) const;

    private:
        Buffer* indexBuffer;
        size_t indexOffset;
        VkIndexType indexType;

        std::vector<Buffer*> vertexBuffers;
        std::vector<VkDeviceSize> vertexOffsets;
    };

    template <typename T>
    void Mesh::SetVertexAttrib(CommandBuffer *commandBuffer, const std::vector<T> &attrib, uint32_t index) {
        // prepare vertex buffer
        Buffer* buffer = new VertexBuffer(commandBuffer->GetDevice(), BufferSize(attrib));
        commandBuffer->CopyDataToBuffer(attrib, buffer);

        SetVertexAttrib(buffer, 0, index);
    }

    template <typename T>
    void Mesh::SetIndexAttrib(CommandBuffer *commandBuffer, const std::vector<T> &attrib) {
        // prepare index buffer
        Buffer* buffer = new IndexBuffer(commandBuffer->GetDevice(), BufferSize(attrib));
        commandBuffer->CopyDataToBuffer(attrib, buffer);

        SetIndexAttrib(buffer, 0, sizeof(T) == 4 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
    }

} // end of namespace slim

#endif // end of SLIM_UTILITY_MESH_H
