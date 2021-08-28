#ifndef SLIM_UTILITY_MESH_H
#define SLIM_UTILITY_MESH_H

#include <variant>
#include "core/buffer.h"
#include "core/commands.h"
#include "utility/interface.h"
#include "utility/transform.h"
#include "utility/boundingbox.h"

namespace slim {

    class Scene;

    using DrawCommand = VkDrawIndirectCommand;
    using DrawIndexed = VkDrawIndexedIndirectCommand;
    using DrawVariant = std::variant<DrawCommand, DrawIndexed>;

    // mesh
    // lowest level building blocks
    class Mesh : public NotCopyable, public NotMovable, public ReferenceCountable {
        friend class Scene;
    public:

        template <typename VertexType>
        VertexType* AllocateVertexBuffer(size_t vertexCount) {
            vertexData.resize(vertexCount * sizeof(VertexType));
            this->vertexCount = vertexCount;
            return reinterpret_cast<VertexType*>(vertexData.data());
        }

        template <typename VertexType>
        VertexType* SetVertexBuffer(const std::vector<VertexType>& data) {
            VertexType* dst = AllocateVertexBuffer<VertexType>(data.size());
            std::memcpy(dst, data.data(), sizeof(VertexType) * data.size());
            return dst;
        }

        template <typename VertexType>
        VertexType* GetVertexData() {
            return vertexData.data();
        }

        template <typename IndexType>
        IndexType* AllocateIndexBuffer(size_t indexCount) {
            indexData.resize(indexCount * sizeof(IndexType));
            #ifndef NDBUEG
            if (!std::is_same<IndexType, uint16_t>::value &&
                !std::is_same<IndexType, uint32_t>::value) {
                throw std::runtime_error("[Mesh] index type must be uint16_t or uint32_t");
            }
            #endif
            indexType = sizeof(IndexType) == sizeof(uint32_t) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
            this->indexCount = indexCount;
            return reinterpret_cast<IndexType*>(indexData.data());
        }

        template <typename IndexType>
        IndexType* SetIndexBuffer(const std::vector<IndexType>& data) {
            IndexType* dst = AllocateIndexBuffer<IndexType>(data.size());
            std::memcpy(dst, data.data(), sizeof(IndexType) * data.size());
            return dst;
        }

        template <typename IndexType>
        IndexType* GetIndexData() {
            return indexData.data();
        }

        VkIndexType GetIndexType() const {
            return indexType;
        }

        uint64_t GetVertexCount() const {
            return vertexCount;
        }

        uint64_t GetIndexCount() const {
            return indexCount;
        }

        void SetBoundingBox(const BoundingBox& box) {
            aabb = box;
        }

        BoundingBox GetBoundingBox(const Transform& transform) const {
            return transform.LocalToWorld() * aabb;
        }

        // This method is for automatic vertex buffer binding
        void AddInputBinding(uint32_t binding, uint32_t offset);

        VkAabbPositionsKHR GetAabbPositions() const;

        void Bind(CommandBuffer* commandBuffer) const;

    private:
        // raw data
        uint64_t vertexCount;
        uint64_t indexCount = 0;
        std::vector<uint8_t> vertexData = {};
        std::vector<uint8_t> indexData = {};
        VkIndexType indexType;
        std::vector<uint32_t> relativeOffsets = {};  // binding offsets
        BoundingBox aabb;

        // for binding vertex and index buffers (filled by builder)
        std::vector<VkBuffer> vertexBuffers = {};
        std::vector<uint64_t> vertexOffsets = {};
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        uint64_t indexOffset = 0;
        uint64_t vertexOffset = 0;
        DrawVariant drawCall;

        #ifndef NDBUEG
        bool built = false;
        bool hasVertexAttribs = false;
        #endif
    };

} // end of namespace slim

#endif // SLIM_UTILITY_MESH_H
