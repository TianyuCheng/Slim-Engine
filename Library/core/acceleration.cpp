#include "core/commands.h"
#include "core/acceleration.h"

using namespace slim;

accel::AccelStruct::AccelStruct(Geometry* geometry)
    : device(geometry->device) {

    VkBufferCreateFlags bufferFlags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
    VmaMemoryUsage memoryFlags = VMA_MEMORY_USAGE_GPU_ONLY;
    buffer = SlimPtr<Buffer>(device, geometry->sizeInfo.accelerationStructureSize, bufferFlags, memoryFlags);

    VkAccelerationStructureCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    createInfo.createFlags = geometry->createFlags;
    createInfo.buffer = *buffer;
    createInfo.deviceAddress = {};
    createInfo.offset = 0;
    createInfo.size = geometry->sizeInfo.accelerationStructureSize;
    createInfo.pNext = nullptr;

    ErrorCheck(DeviceDispatch(vkCreateAccelerationStructureKHR(*device, &createInfo, nullptr, &handle)), "create acceleration structure");

    address = device->GetDeviceAddress(this);
}

accel::AccelStruct::AccelStruct(Instance* instance)
    : device(instance->device) {

    VkBufferCreateFlags bufferFlags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
    VmaMemoryUsage memoryFlags = VMA_MEMORY_USAGE_GPU_ONLY;
    buffer = SlimPtr<Buffer>(device, instance->sizeInfo.accelerationStructureSize, bufferFlags, memoryFlags);

    VkAccelerationStructureCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    createInfo.createFlags = instance->createFlags;
    createInfo.buffer = *buffer;
    createInfo.deviceAddress = {};
    createInfo.offset = 0;
    createInfo.size = instance->sizeInfo.accelerationStructureSize;
    createInfo.pNext = nullptr;

    ErrorCheck(DeviceDispatch(vkCreateAccelerationStructureKHR(*device, &createInfo, nullptr, &handle)), "create acceleration structure");
}

accel::AccelStruct::~AccelStruct() {
    if (handle) {
        DeviceDispatch(vkDestroyAccelerationStructureKHR(*device, handle, nullptr));
        handle = VK_NULL_HANDLE;
    }
}

accel::Instance::Instance(Device* device, VkAccelerationStructureCreateFlagsKHR createFlags) : device(device), createFlags(createFlags) {
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
}

void accel::Instance::AddInstance(Buffer* instanceBuffer, uint64_t instanceOffset, uint64_t instanceCount) {
    // prepare geometry data
    geometries.push_back(VkAccelerationStructureGeometryKHR { });
    auto& geometry = geometries.back();
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.flags = 0;
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.data.deviceAddress = device->GetDeviceAddress(instanceBuffer) + instanceOffset;
    geometry.geometry.instances.arrayOfPointers = VK_FALSE;

    // prepare geometry offsets
    buildRanges.push_back(VkAccelerationStructureBuildRangeInfoKHR {});
    auto& buildRange = buildRanges.back();
    buildRange.primitiveCount = instanceCount;
    buildRange.primitiveOffset = 0;
    buildRange.firstVertex = 0;
    buildRange.transformOffset = 0;
}

void accel::Instance::Instance::Prepare() {
    VkAccelerationStructureBuildTypeKHR buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

    buildInfo.geometryCount = geometries.size();
    buildInfo.pGeometries = geometries.data();
    buildInfo.ppGeometries = nullptr;

    // query build size
    std::vector<uint32_t> maxPrimitiveCount = {};
    for (const auto& range : buildRanges) {
        maxPrimitiveCount.push_back(range.primitiveCount);
    }
    DeviceDispatch(vkGetAccelerationStructureBuildSizesKHR(*device, buildType, &buildInfo, maxPrimitiveCount.data(), &sizeInfo));
}

accel::Geometry::Geometry(Device* device, VkAccelerationStructureCreateFlagsKHR createFlags) : device(device), createFlags(createFlags) {
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
}

void accel::Geometry::Geometry::AddTriangles(Buffer* indexBuffer, uint64_t indexOffset, uint64_t indexCount, VkIndexType indexType,
                                             Buffer* vertexBuffer, uint64_t vertexOffset, uint64_t vertexStride,
                                             Buffer* transformBuffer, uint64_t transformOffset,
                                             VkFormat vertexFormat) {

    geometries.push_back(VkAccelerationStructureGeometryKHR { });
    auto& geometry = geometries.back();

    // prepare geometry data
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.flags = 0;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.triangles.indexType = indexType;
    geometry.geometry.triangles.indexData.deviceAddress = device->GetDeviceAddress(indexBuffer) + indexOffset;
    geometry.geometry.triangles.vertexData.deviceAddress = device->GetDeviceAddress(vertexBuffer) + vertexOffset;
    geometry.geometry.triangles.vertexStride = vertexStride;
    geometry.geometry.triangles.vertexFormat = vertexFormat;
    geometry.geometry.triangles.transformData.deviceAddress = transformBuffer ? device->GetDeviceAddress(transformBuffer) : 0;

    // prepare geometry offsets
    buildRanges.push_back(VkAccelerationStructureBuildRangeInfoKHR {});
    auto& buildRange = buildRanges.back();
    buildRange.primitiveCount = indexCount / 3;
    buildRange.primitiveOffset = 0;
    buildRange.firstVertex = 0;
    buildRange.transformOffset = transformOffset;
}

void accel::Geometry::Geometry::Prepare() {
    VkAccelerationStructureBuildTypeKHR buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

    buildInfo.geometryCount = geometries.size();
    buildInfo.pGeometries = geometries.data();
    buildInfo.ppGeometries = nullptr;

    // query build size
    std::vector<uint32_t> maxPrimitiveCount = {};
    for (const auto& range : buildRanges) {
        maxPrimitiveCount.push_back(range.primitiveCount);
    }
    DeviceDispatch(vkGetAccelerationStructureBuildSizesKHR(*device, buildType, &buildInfo, maxPrimitiveCount.data(), &sizeInfo));
}
