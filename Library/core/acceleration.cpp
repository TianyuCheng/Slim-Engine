#include "core/acceleration.h"

using namespace slim;

AccelerationStructure::AccelerationStructure(Device* device, VkAccelerationStructureCreateFlagsKHR createFlags)
    : device(device), createFlags(createFlags) {
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
}

AccelerationStructure::~AccelerationStructure() {
    if (handle) {
        DeviceDispatch(vkDestroyAccelerationStructureKHR(*device, handle, nullptr));
        handle = VK_NULL_HANDLE;
    }
}

void AccelerationStructure::AddInstance(Buffer* instanceBuffer, uint64_t instanceOffset, uint64_t instanceCount) {
    geometries.push_back(VkAccelerationStructureGeometryKHR { });
    auto& geometry = geometries.back();
    geometry.flags = 0;
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.data.deviceAddress = GetDeviceAddress(*device, *instanceBuffer) + instanceOffset;
    geometry.geometry.instances.arrayOfPointers = VK_FALSE;

    buildRanges.push_back(VkAccelerationStructureBuildRangeInfoKHR {});
    auto& buildRange = buildRanges.back();
    buildRange.primitiveCount = instanceCount;
    buildRange.primitiveOffset = 0;
    buildRange.firstVertex = 0;
    buildRange.transformOffset = 0;

    tlas = true;
}

void AccelerationStructure::AddBoundingBox(Buffer* aabbBuffer, uint64_t offset, uint64_t stride) {
    #ifndef NDEBUG
    if (stride % 8 != 0) {
        throw std::runtime_error("aabb stride must be a multiple of 8");
    }
    #endif

    geometries.push_back(VkAccelerationStructureGeometryKHR { });
    auto& geometry = geometries.back();
    geometry.flags = 0;
    geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
    geometry.geometry.aabbs.data.deviceAddress = GetDeviceAddress(*device, *aabbBuffer) + offset;
    geometry.geometry.aabbs.stride = stride;

    buildRanges.push_back(VkAccelerationStructureBuildRangeInfoKHR {});
    auto& buildRange = buildRanges.back();
    buildRange.primitiveCount = 1;
    buildRange.primitiveOffset = 0;
    buildRange.firstVertex = 0;
    buildRange.transformOffset = 0;

    tlas = true;
}

void AccelerationStructure::AddTriangles(Buffer* indexBuffer, uint64_t indexOffset, uint64_t indexCount, VkIndexType indexType,
                                         Buffer* vertexBuffer, uint64_t vertexOffset, uint64_t vertexStride,
                                         Buffer* transformBuffer, uint64_t transformOffset,
                                         VkFormat vertexFormat) {

    geometries.push_back(VkAccelerationStructureGeometryKHR { });
    auto& geometry = geometries.back();

    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.flags = 0;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.triangles.indexType = indexType;
    geometry.geometry.triangles.indexData.deviceAddress = GetDeviceAddress(*device, *indexBuffer) + indexOffset;
    geometry.geometry.triangles.vertexData.deviceAddress = GetDeviceAddress(*device, *vertexBuffer) + vertexOffset;
    geometry.geometry.triangles.vertexStride = vertexStride;
    geometry.geometry.triangles.vertexFormat = vertexFormat;
    geometry.geometry.triangles.transformData.deviceAddress = transformBuffer ? GetDeviceAddress(*device, *transformBuffer) : 0;

    buildRanges.push_back(VkAccelerationStructureBuildRangeInfoKHR {});
    auto& buildRange = buildRanges.back();
    buildRange.primitiveCount = indexCount / 3;
    buildRange.primitiveOffset = 0;
    buildRange.firstVertex = 0;
    buildRange.transformOffset = transformOffset;

    blas = true;
}

void AccelerationStructure::Prepare() {
    bool build = handle == VK_NULL_HANDLE;

    VkAccelerationStructureBuildTypeKHR buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

    buildInfo.geometryCount = geometries.size();
    buildInfo.pGeometries = geometries.data();
    buildInfo.ppGeometries = nullptr;

    std::vector<uint32_t> maxPrimitiveCount = {};
    for (const auto& range : buildRanges) {
        maxPrimitiveCount.push_back(range.primitiveCount);
    }

    // query build size
    VkAccelerationStructureBuildSizesInfoKHR sizeInfo = {};
    sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    DeviceDispatch(vkGetAccelerationStructureBuildSizesKHR(*device, buildType, &buildInfo, maxPrimitiveCount.data(), &sizeInfo));

    // allocate scratch space
    scratchSize = build ? sizeInfo.buildScratchSize : sizeInfo.updateScratchSize;

    // create acceleration structure handle
    if (handle == VK_NULL_HANDLE) {
        asBuffer = SlimPtr<Buffer>(device, sizeInfo.accelerationStructureSize,
                                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                                   VMA_MEMORY_USAGE_GPU_ONLY);

        VkAccelerationStructureCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        createInfo.createFlags = createFlags;
        createInfo.type = tlas ? VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR
                               : VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        createInfo.buffer = *asBuffer;
        createInfo.offset = 0;
        createInfo.deviceAddress = {};
        ErrorCheck(DeviceDispatch(vkCreateAccelerationStructureKHR(*device, &createInfo, nullptr, &handle)), "create acceleration structure");
    }

    // update build info
    buildInfo.srcAccelerationStructure = build ? VK_NULL_HANDLE : handle;
    buildInfo.dstAccelerationStructure = handle;
}
