#ifndef SLIM_CORE_ACCELERATION_H
#define SLIM_CORE_ACCELERATION_H

#include "core/vulkan.h"
#include "core/debug.h"
#include "core/query.h"
#include "core/device.h"
#include "core/buffer.h"
#include "core/vkutils.h"
#include "utility/interface.h"

namespace slim::scene {
    class Builder;
    class Node;
}

namespace slim::accel {

    class AccelStruct;
    class Instance;
    class Geometry;
    class Builder;

    // ------------------------------------------------------------

    class AccelStruct : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkAccelerationStructureKHR> {
    public:
        explicit AccelStruct(Instance* instance);
        explicit AccelStruct(Geometry* geometry);
        virtual ~AccelStruct();

        void SetName(const std::string& name) const;
    private:
        SmartPtr<Device> device;
        SmartPtr<Buffer> buffer;
        VkDeviceAddress address;
    }; // end of Structure

    // ------------------------------------------------------------

    class Instance : public NotCopyable, public NotMovable, public ReferenceCountable {
        friend class AccelStruct;
    public:
        explicit Instance(Device* device, VkAccelerationStructureCreateFlagsKHR createFlags = 0);
        uint32_t AddInstance(scene::Node* node, uint32_t sbtRecordOffset = 0, uint32_t mask = 0xff);
        void Prepare(bool update);

    private:
        void PrepareInstanceBuffer();

    private:
        SmartPtr<Device> device;
        SmartPtr<Buffer> buffer;
        std::vector<VkAccelerationStructureInstanceKHR> instances;
        bool modified = false;

    public:
        SmartPtr<AccelStruct> accel;
        VkAccelerationStructureCreateFlagsKHR createFlags;
        std::vector<VkAccelerationStructureGeometryKHR> geometries;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRanges;
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo = {};
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
    };

    // ------------------------------------------------------------

    class Geometry : public NotCopyable, public NotMovable, public ReferenceCountable {
        friend class AccelStruct;
    public:
        explicit Geometry(Device* device, VkAccelerationStructureCreateFlagsKHR createFlags = 0);
        void AddTriangles(Buffer* indexBuffer, uint64_t indexOffset, uint64_t indexCount, VkIndexType indexType,
                          Buffer* vertexBuffer, uint64_t vertexOffset, uint64_t vertexCount, uint64_t vertexStride,
                          Buffer* transformBuffer = nullptr, uint64_t transformOffset = 0,
                          VkFormat vertexFormat = VK_FORMAT_R32G32B32_SFLOAT);
        void AddAABBs(Buffer* aabbsBuffer, uint32_t count, uint32_t stride);
        void Prepare(bool update);

    private:
        SmartPtr<Device> device;

    public:
        SmartPtr<AccelStruct> accel;
        SmartPtr<AccelStruct> cleanupAccel;
        VkAccelerationStructureCreateFlagsKHR createFlags;
        std::vector<VkAccelerationStructureGeometryKHR> geometries;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRanges;
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo = {};
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
    };

} // end of namespace slim

#endif // SLIM_CORE_ACCELERATION_H
