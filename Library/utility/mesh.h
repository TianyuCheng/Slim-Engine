#ifndef SLIM_UTILITY_MESH_H
#define SLIM_UTILITY_MESH_H

#include <variant>
#include "core/buffer.h"
#include "core/commands.h"
#include "core/acceleration.h"
#include "utility/interface.h"
#include "utility/transform.h"
#include "utility/boundingbox.h"

namespace slim {

    namespace scene {
        class Node;
        class Builder;
    };

    using DrawCommand = VkDrawIndirectCommand;
    using DrawIndexed = VkDrawIndexedIndirectCommand;
    using DrawVariant = std::variant<DrawCommand, DrawIndexed>;

    using VertexData = std::vector<std::vector<uint8_t>>;
    using VertexOffset = std::vector<uint64_t>;
    using IndexData = std::vector<uint8_t>;

    // mesh
    // lowest level building blocks
    class Mesh : public NotCopyable, public NotMovable, public ReferenceCountable {
        friend class scene::Node;
        friend class scene::Builder;
    public:

        template <typename VertexType>
        VertexType* AllocateVertexBuffer(size_t vertexCount, size_t binding) {
            if (vertexData.size() <= binding) {
                vertexData.resize(binding + 1);
            }
            vertexData[binding].resize(vertexCount * sizeof(VertexType));
            #ifndef NDEBUG
            if (this->vertexCount != 0 && this->vertexCount != vertexCount) {
                throw std::runtime_error("inconsistent vertex count for vertex buffers!");
            }
            hasVertexAttribs = true;
            #endif
            // we assume position is in the first binding with offset 0
            if (binding == 0) {
                vertexStride = sizeof(VertexType);
            }
            this->vertexCount = vertexCount;
            return reinterpret_cast<VertexType*>(vertexData[binding].data());
        }

        template <typename VertexType>
        VertexType* GetVertexData(uint32_t binding) {
            return vertexData[binding].data();
        }

        template <typename VertexType>
        VertexType* SetVertexBuffer(const std::vector<VertexType>& data, size_t binding = 0) {
            VertexType* dst = AllocateVertexBuffer<VertexType>(data.size(), binding);
            std::memcpy(dst, data.data(), sizeof(VertexType) * data.size());
            return dst;
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
        IndexType* GetIndexData() {
            return indexData.data();
        }

        template <typename IndexType>
        IndexType* SetIndexBuffer(const std::vector<IndexType>& data) {
            IndexType* dst = AllocateIndexBuffer<IndexType>(data.size());
            std::memcpy(dst, data.data(), sizeof(IndexType) * data.size());
            return dst;
        }

        VkIndexType GetIndexType() const {
            return indexType;
        }

        uint32_t GetVertexStride() const {
            return vertexStride;
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

        VkAabbPositionsKHR GetAabbPositions() const;

        void Bind(CommandBuffer* commandBuffer) const;

        std::tuple<Buffer*, uint64_t> GetVertexBuffer(uint32_t binding) const {
            return std::make_tuple(vertexBuffer, vertexOffsets[0]);
        }

        std::tuple<Buffer*, uint64_t> GetIndexBuffer() const {
            return std::make_tuple(indexBuffer, indexOffset);
        }

        // for raytracing
        SmartPtr<accel::Geometry> blas = nullptr;

    private:
        // index data
        uint64_t indexCount = 0;
        uint64_t indexOffset = 0;
        IndexData indexData = {};
        VkIndexType indexType = VK_INDEX_TYPE_UINT32;
        SmartPtr<Buffer> indexBuffer = nullptr;

        // vertex data
        uint64_t vertexCount = 0;
        uint32_t vertexStride = 0;
        VertexData vertexData = {};
        VertexOffset vertexOffsets = {};
        SmartPtr<Buffer> vertexBuffer = nullptr; // for automatic destroy
        std::vector<VkBuffer> vertexBuffers = {};

        // bounding box
        BoundingBox aabb;

        // draw call information
        DrawVariant drawCall;

        #ifndef NDBUEG
        bool built = false;
        bool hasVertexAttribs = false;
        #endif
    };

} // end of namespace slim

#endif // SLIM_UTILITY_MESH_H
