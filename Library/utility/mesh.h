#ifndef SLIM_UTILITY_MESH_H
#define SLIM_UTILITY_MESH_H

#include "core/buffer.h"
#include "core/commands.h"
#include "utility/interface.h"

namespace slim {

    class Mesh final : public ReferenceCountable {
        friend struct Submesh;
    public:
        template <typename T>
        void SetVertexAttrib(CommandBuffer *commandBuffer, const std::vector<T> &attrib, uint32_t index);

        template <typename T>
        void SetIndexAttrib(CommandBuffer *commandBuffer, const std::vector<T> &attrib);

    private:
        SmartPtr<IndexBuffer> indexBuffer;
        std::vector<SmartPtr<VertexBuffer>> vertexBuffers;
        std::vector<size_t> vertexElemSizes;
        size_t indexElemSize = 4;
    };

    template <typename T>
    void Mesh::SetVertexAttrib(CommandBuffer *commandBuffer, const std::vector<T> &attrib, uint32_t index) {
        // prepare vertex buffer
        if (vertexBuffers.size() <= index) { vertexBuffers.resize(index + 1); }
        if (vertexElemSizes.size() <= index) { vertexElemSizes.resize(index + 1); }

        vertexBuffers[index] = SlimPtr<VertexBuffer>(commandBuffer->GetContext(), BufferSize(attrib));
        vertexElemSizes[index] = sizeof(T);

        // copy data to vertex buffer
        commandBuffer->CopyDataToBuffer(attrib, vertexBuffers[index]);
    }

    template <typename T>
    void Mesh::SetIndexAttrib(CommandBuffer *commandBuffer, const std::vector<T> &attrib) {
        // prepare index buffer
        indexBuffer = SlimPtr<IndexBuffer>(commandBuffer->GetContext(), BufferSize(attrib));
        indexElemSize = sizeof(T);

        // copy data to index buffer
        commandBuffer->CopyDataToBuffer(attrib, indexBuffer);
    }

    struct Submesh {
        SmartPtr<Mesh> mesh;
        size_t vertexOffset = 0;
        size_t indexOffset = 0;
        size_t indexCount = 0;
        size_t instanceCount = 1;

        explicit Submesh();
        explicit Submesh(Mesh *mesh, size_t vertexOffset, size_t indexOffset, size_t indexCount);
        virtual ~Submesh() = default;

        void Bind(CommandBuffer *commandBuffer) const;
        void Draw(CommandBuffer *commandBuffer) const;
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_MESH_H
