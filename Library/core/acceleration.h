#ifndef SLIM_CORE_ACCELERATION_H
#define SLIM_CORE_ACCELERATION_H

#include "core/vulkan.h"
#include "core/debug.h"
#include "core/device.h"
#include "core/buffer.h"
#include "core/vkutils.h"
#include "utility/interface.h"

namespace slim {

    class AccelerationStructure : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkAccelerationStructureKHR> {
        friend class CommandBuffer;
    public:
        explicit AccelerationStructure(Device* device, VkAccelerationStructureCreateFlagsKHR createFlags);
        virtual ~AccelerationStructure();

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
        SmartPtr<Buffer> asBuffer;
        VkAccelerationStructureCreateFlagsKHR createFlags;

        uint32_t scratchSize = 0;
        bool blas = false, tlas = false;
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
        std::vector<VkAccelerationStructureGeometryKHR> geometries;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRanges;

    }; // end of AccelerationStructure

} // end of namespace slim

#endif // SLIM_CORE_ACCELERATION_H
