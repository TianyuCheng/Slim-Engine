#ifndef SLIM_CORE_ACCELERATION_H
#define SLIM_CORE_ACCELERATION_H

#include "core/vulkan.h"
#include "core/debug.h"
#include "core/query.h"
#include "core/device.h"
#include "core/buffer.h"
#include "core/vkutils.h"
#include "utility/interface.h"

namespace slim {

    class AccelerationStructure;
    class AccelerationStructureInput;
    class AccelerationStructureBuilder;

    class AccelerationStructure : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkAccelerationStructureKHR> {
        friend class AccelerationStructureInput;
        friend class AccelerationStructureBuilder;
    public:
        explicit AccelerationStructure(AccelerationStructureInput* input);
        virtual ~AccelerationStructure();

    private:
        SmartPtr<Device> device;
        SmartPtr<Buffer> buffer;
    }; // end of AccelerationStructure

    // ------------------------------------------------------------

    class AccelerationStructureInput {
        friend class AccelerationStructure;
        friend class AccelerationStructureBuilder;
    public:
        explicit AccelerationStructureInput(Device* device, VkAccelerationStructureCreateFlagsKHR createFlags);

        void AddInstance(Buffer* instanceBuffer, uint64_t instanceOffset, uint64_t instanceCount);

        void AddBoundingBox(Buffer* aabbBuffer, uint64_t offset, uint64_t stride);

        void AddTriangles(Buffer* indexBuffer, uint64_t indexOffset, uint64_t indexCount, VkIndexType indexType,
                          Buffer* vertexBuffer, uint64_t vertexOffset, uint64_t vertexStride,
                          Buffer* transformBuffer = nullptr, uint64_t transformOffset = 0,
                          VkFormat vertexFormat = VK_FORMAT_R32G32B32_SFLOAT);

    private:
        void Prepare();

    private:
        SmartPtr<Device> device;
        VkAccelerationStructureTypeKHR type;
        VkAccelerationStructureCreateFlagsKHR createFlags;
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo = {};
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
        std::vector<VkAccelerationStructureGeometryKHR> geometries;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRanges;
    }; // end of AccelerationStructureInput

    // ------------------------------------------------------------

    class AccelerationStructureBuilder {
    public:
        explicit AccelerationStructureBuilder(Device* device);

        void EnableCompaction();

        void BuildTlas(AccelerationStructureInput* input);
        void BuildBlas(AccelerationStructureInput* input);
        void BuildBlas(const std::vector<AccelerationStructureInput*>& inputs);

    private:

        void CreateTlas(CommandBuffer* commandBuffer,
                        AccelerationStructureInput* input,
                        VkDeviceAddress scratchAddress);
        void CreateBlas(CommandBuffer* commandBuffer,
                        const std::vector<uint32_t> &indices,
                        const std::vector<AccelerationStructureInput*>& inputs,
                        VkDeviceAddress scratchAddress, QueryPool* queryPool);
        void CompactBlas(CommandBuffer* commandBuffer,
                         const std::vector<uint32_t> &indices,
                         const std::vector<AccelerationStructureInput*>& inputs,
                         QueryPool* queryPool);

    private:
        SmartPtr<Device> device;
        bool compaction = false;
        SmartPtr<AccelerationStructure> tlas;
        std::vector<SmartPtr<AccelerationStructure>> blas;
    };

} // end of namespace slim

#endif // SLIM_CORE_ACCELERATION_H
