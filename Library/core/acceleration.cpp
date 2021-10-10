#include "core/commands.h"
#include "core/acceleration.h"
#include "utility/scenegraph.h"

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

void accel::AccelStruct::SetName(const std::string& name) const {
    if (device->IsDebuggerEnabled()) {
        VkDebugMarkerObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR_EXT;
        nameInfo.object = (uint64_t) handle;
        nameInfo.pObjectName = name.c_str();
        ErrorCheck(DeviceDispatch(vkDebugMarkerSetObjectNameEXT(*device, &nameInfo)), "set acceleration structure name");
    }
    if (buffer) {
        buffer->SetName(name + " buffer");
    }
}

// ------------------------------------------------------------

accel::Instance::Instance(Device* device, VkAccelerationStructureCreateFlagsKHR createFlags) : device(device), createFlags(createFlags) {
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
}

uint32_t accel::Instance::AddInstance(scene::Node* node, uint32_t sbtRecordOffset, uint32_t mask) {
    uint32_t offset =  instances.size();
    if (!node->HasDraw()) {
        return offset;
    }
    uint32_t instanceId = offset;
    VkTransformMatrixKHR transform = node->GetVkTransformMatrix();
    for (auto& [mesh, _] : *node) {
        accel::AccelStruct* as = mesh->GetBlasGeometry()->accel;
        #ifndef NDEBUG
        if (as == nullptr) {
            throw std::runtime_error("node's geometry blas has not been built!");
        }
        #endif
        instances.push_back(VkAccelerationStructureInstanceKHR { });
        auto& instance = instances.back();
        instance.transform = transform;
        instance.instanceCustomIndex = instanceId++;
        instance.accelerationStructureReference = device->GetDeviceAddress(as);
        instance.instanceShaderBindingTableRecordOffset = sbtRecordOffset;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.mask = mask;
        modified = true;
    }
    return offset;
}

void accel::Instance::PrepareInstanceBuffer() {
    uint32_t instanceCount = instances.size();
    uint32_t instanceOffset = 0;

    if (!buffer.get()) {
        uint64_t bufferSize = instanceCount * sizeof(VkAccelerationStructureInstanceKHR);
        VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                                       | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
        buffer = SlimPtr<Buffer>(device, bufferSize, bufferUsage, memoryUsage);
        buffer->SetName("RT Instance Data");
    }

    // update buffer data
    if (modified) {
        device->Execute([&](CommandBuffer* commandBuffer) {
            commandBuffer->CopyDataToBuffer(instances, buffer);
        });
        modified = false;
    }

    // clear data
    geometries.clear();
    buildRanges.clear();

    // update geometry
    geometries.push_back(VkAccelerationStructureGeometryKHR { });
    auto& geometry = geometries.back();
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR; // TODO: force opaque objcet for now
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.data.deviceAddress = device->GetDeviceAddress(buffer);
    geometry.geometry.instances.arrayOfPointers = VK_FALSE;

    // prepare geometry offsets
    buildRanges.push_back(VkAccelerationStructureBuildRangeInfoKHR {});
    auto& buildRange = buildRanges.back();
    buildRange.primitiveCount = instanceCount;
    buildRange.primitiveOffset = instanceOffset;
    buildRange.firstVertex = 0;
    buildRange.transformOffset = 0;

    #ifndef NDEBUG
    if (geometries.size() > 1) {
        std::cerr << "[ERROR] top level acceleration structure receives multiple instances addition" << std::endl;
        throw std::runtime_error("[ERROR] top level acceleration structure receives multiple instances addition");
    }
    #endif
}

void accel::Instance::Prepare(bool update) {
    PrepareInstanceBuffer();

    VkAccelerationStructureBuildTypeKHR buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

    buildInfo.geometryCount = geometries.size();
    buildInfo.pGeometries = geometries.data();
    buildInfo.ppGeometries = nullptr;

    // query build size
    std::vector<uint32_t> maxPrimitiveCount = {};
    for (const auto& range : buildRanges) {
        maxPrimitiveCount.push_back(range.primitiveCount);
    }

    buildInfo.mode = update
                   ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR
                   : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    DeviceDispatch(vkGetAccelerationStructureBuildSizesKHR(*device, buildType, &buildInfo, maxPrimitiveCount.data(), &sizeInfo));
}

// ------------------------------------------------------------

accel::Geometry::Geometry(Device* device, VkAccelerationStructureCreateFlagsKHR createFlags) : device(device), createFlags(createFlags) {
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
}

void accel::Geometry::AddTriangles(Buffer* indexBuffer, uint64_t indexOffset, uint64_t indexCount, VkIndexType indexType,
                                   Buffer* vertexBuffer, uint64_t vertexOffset, uint64_t vertexCount, uint64_t vertexStride,
                                   Buffer* transformBuffer, uint64_t transformOffset,
                                   VkFormat vertexFormat) {

    geometries.push_back(VkAccelerationStructureGeometryKHR { });
    auto& geometry = geometries.back();

    // prepare geometry data
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR; // TODO: force opaque objcet for now
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.triangles.indexType = indexType;
    geometry.geometry.triangles.indexData.deviceAddress = device->GetDeviceAddress(indexBuffer) + indexOffset;      // NOTE: This is probably wrong
    geometry.geometry.triangles.vertexStride = vertexStride;
    geometry.geometry.triangles.vertexFormat = vertexFormat;
    geometry.geometry.triangles.maxVertex = vertexCount;
    geometry.geometry.triangles.vertexData.deviceAddress = device->GetDeviceAddress(vertexBuffer) + vertexOffset;   // NOTE: This is probably wrong
    geometry.geometry.triangles.transformData.deviceAddress = transformBuffer ? device->GetDeviceAddress(transformBuffer) : 0;

    // prepare geometry offsets
    buildRanges.push_back(VkAccelerationStructureBuildRangeInfoKHR {});
    auto& buildRange = buildRanges.back();
    buildRange.primitiveCount = indexCount / 3;
    buildRange.primitiveOffset = 0;
    buildRange.firstVertex = 0;
    buildRange.transformOffset = transformOffset;
}

void accel::Geometry::Prepare(bool update) {
    VkAccelerationStructureBuildTypeKHR buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

    buildInfo.geometryCount = geometries.size();
    buildInfo.pGeometries = geometries.data();
    buildInfo.ppGeometries = nullptr;

    // query build size
    std::vector<uint32_t> maxPrimitiveCount = {};
    for (const auto& range : buildRanges) {
        maxPrimitiveCount.push_back(range.primitiveCount);
    }
    buildInfo.mode = update
                   ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR
                   : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    DeviceDispatch(vkGetAccelerationStructureBuildSizesKHR(*device, buildType, &buildInfo, maxPrimitiveCount.data(), &sizeInfo));
}

void accel::Geometry::AddAABBs(Buffer* aabbsBuffer, uint32_t count, uint32_t stride) {
    geometries.push_back(VkAccelerationStructureGeometryKHR { });
    auto& geometry = geometries.back();

    // prepare geometry data
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR; // TODO: force opaque objcet for now
    geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
    geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    geometry.geometry.aabbs.pNext = nullptr;
    geometry.geometry.aabbs.stride = stride;
    geometry.geometry.aabbs.data.deviceAddress = device->GetDeviceAddress(aabbsBuffer);

    // prepare geometry offsets
    buildRanges.push_back(VkAccelerationStructureBuildRangeInfoKHR {});
    auto& buildRange = buildRanges.back();
    buildRange.primitiveCount = count;
    buildRange.primitiveOffset = 0;
    buildRange.firstVertex = 0;
    buildRange.transformOffset = 0;
}
