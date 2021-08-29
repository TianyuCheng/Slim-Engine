#include "core/commands.h"
#include "core/acceleration.h"

using namespace slim;

AccelerationStructure::AccelerationStructure(AccelerationStructureInput* input)
    : device(input->device) {

    VkBufferCreateFlags bufferFlags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
    VmaMemoryUsage memoryFlags = VMA_MEMORY_USAGE_GPU_ONLY;
    buffer = SlimPtr<Buffer>(device, input->sizeInfo.accelerationStructureSize, bufferFlags, memoryFlags);

    VkAccelerationStructureCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    createInfo.createFlags = input->createFlags;
    createInfo.buffer = *buffer;
    createInfo.deviceAddress = {};
    createInfo.offset = 0;
    createInfo.size = input->sizeInfo.accelerationStructureSize;
    createInfo.pNext = nullptr;

    ErrorCheck(DeviceDispatch(vkCreateAccelerationStructureKHR(*device, &createInfo, nullptr, &handle)), "create acceleration structure");
}

AccelerationStructure::~AccelerationStructure() {
    if (handle) {
        DeviceDispatch(vkDestroyAccelerationStructureKHR(*device, handle, nullptr));
        handle = VK_NULL_HANDLE;
    }
}

AccelerationStructureInput::AccelerationStructureInput(Device* device, VkAccelerationStructureCreateFlagsKHR createFlags) : device(device), createFlags(createFlags) {
}

void AccelerationStructureInput::AddInstance(Buffer* instanceBuffer, uint64_t instanceOffset, uint64_t instanceCount) {
    // prepare geometry data
    geometries.push_back(VkAccelerationStructureGeometryKHR { });
    auto& geometry = geometries.back();
    geometry.flags = 0;
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.data.deviceAddress = GetDeviceAddress(*device, *instanceBuffer) + instanceOffset;
    geometry.geometry.instances.arrayOfPointers = VK_FALSE;

    // prepare geometry offsets
    buildRanges.push_back(VkAccelerationStructureBuildRangeInfoKHR {});
    auto& buildRange = buildRanges.back();
    buildRange.primitiveCount = instanceCount;
    buildRange.primitiveOffset = 0;
    buildRange.firstVertex = 0;
    buildRange.transformOffset = 0;

    type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
}

void AccelerationStructureInput::AddBoundingBox(Buffer* aabbBuffer, uint64_t offset, uint64_t stride) {
    #ifndef NDEBUG
    if (stride % 8 != 0) {
        throw std::runtime_error("aabb stride must be a multiple of 8");
    }
    #endif

    // prepare geometry data
    geometries.push_back(VkAccelerationStructureGeometryKHR { });
    auto& geometry = geometries.back();
    geometry.flags = 0;
    geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
    geometry.geometry.aabbs.data.deviceAddress = GetDeviceAddress(*device, *aabbBuffer) + offset;
    geometry.geometry.aabbs.stride = stride;

    // prepare geometry offsets
    buildRanges.push_back(VkAccelerationStructureBuildRangeInfoKHR {});
    auto& buildRange = buildRanges.back();
    buildRange.primitiveCount = 1;
    buildRange.primitiveOffset = 0;
    buildRange.firstVertex = 0;
    buildRange.transformOffset = 0;

    type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
}

void AccelerationStructureInput::AddTriangles(Buffer* indexBuffer, uint64_t indexOffset, uint64_t indexCount, VkIndexType indexType,
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
    geometry.geometry.triangles.indexData.deviceAddress = GetDeviceAddress(*device, *indexBuffer) + indexOffset;
    geometry.geometry.triangles.vertexData.deviceAddress = GetDeviceAddress(*device, *vertexBuffer) + vertexOffset;
    geometry.geometry.triangles.vertexStride = vertexStride;
    geometry.geometry.triangles.vertexFormat = vertexFormat;
    geometry.geometry.triangles.transformData.deviceAddress = transformBuffer ? GetDeviceAddress(*device, *transformBuffer) : 0;

    // prepare geometry offsets
    buildRanges.push_back(VkAccelerationStructureBuildRangeInfoKHR {});
    auto& buildRange = buildRanges.back();
    buildRange.primitiveCount = indexCount / 3;
    buildRange.primitiveOffset = 0;
    buildRange.firstVertex = 0;
    buildRange.transformOffset = transformOffset;

    type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
}

void AccelerationStructureInput::Prepare() {
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

AccelerationStructureBuilder::AccelerationStructureBuilder(Device* device) : device(device) {
}

void AccelerationStructureBuilder::EnableCompaction() {
    compaction = true;
}

void AccelerationStructureBuilder::BuildTlas(AccelerationStructureInput* input) {
    input->Prepare();

    // scratch buffer
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    auto scratchBuffer = SlimPtr<Buffer>(device, input->sizeInfo.buildScratchSize, bufferUsage, memoryUsage);
    VkDeviceAddress scratchAddress = GetDeviceAddress(*device, *scratchBuffer);

    // create tlas
    device->Execute([&](CommandBuffer* commandBuffer) {
        CreateTlas(commandBuffer, input, scratchAddress);
    });
}

void AccelerationStructureBuilder::BuildBlas(AccelerationStructureInput* input) {
    BuildBlas(std::vector<AccelerationStructureInput*>{ input });
}

void AccelerationStructureBuilder::BuildBlas(const std::vector<AccelerationStructureInput*>& inputs) {
    // prepare acceleration structure input
    for (AccelerationStructureInput* as : inputs) {
        as->Prepare();
    }

    // find data structure sizes
    uint32_t asTotalSize = 0;
    uint32_t maxScratchSize = 0;
    for (AccelerationStructureInput* as : inputs) {
        uint32_t scratchSize = as->sizeInfo.buildScratchSize;
        maxScratchSize = std::max(maxScratchSize, scratchSize);
        asTotalSize += as->sizeInfo.accelerationStructureSize;
    }

    // scratch buffer
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    auto scratchBuffer = SlimPtr<Buffer>(device, maxScratchSize, bufferUsage, memoryUsage);
    VkDeviceAddress scratchAddress = GetDeviceAddress(*device, *scratchBuffer);

    // query pool
    SmartPtr<QueryPool> queryPool = nullptr;
    if (compaction) {
        queryPool = SlimPtr<QueryPool>(device, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, inputs.size(), 0);
    }

    // create acceleration structures
    std::vector<uint32_t> indices;  // Indices of the BLAS to create
    VkDeviceSize batchSize{0};
    VkDeviceSize batchLimit{256'000'000};  // 256 MB
    for (uint32_t i = 0; i < inputs.size(); i++) {
        indices.push_back(i);
        batchSize += inputs[i]->sizeInfo.accelerationStructureSize;

        if (batchSize >= batchLimit || i == inputs.size() - 1) {
            device->Execute([&](CommandBuffer* commandBuffer) {
                CreateBlas(commandBuffer, indices, inputs, scratchAddress, queryPool);
            });

            if (compaction) {
                device->Execute([&](CommandBuffer* commandBuffer) {
                    CompactBlas(commandBuffer, indices, inputs, queryPool);
                });
            }
        } // end of batch size or end of inputs

        batchSize = 0;
        indices.clear();
    } // end of inputs for loop

}

void AccelerationStructureBuilder::CreateTlas(CommandBuffer* commandBuffer,
                                              AccelerationStructureInput* input,
                                              VkDeviceAddress scratchAddress) {
    // 1. create
    tlas = new AccelerationStructure(input);

    // 2. build
    input->buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    input->buildInfo.dstAccelerationStructure = *tlas;
    input->buildInfo.scratchData.deviceAddress = scratchAddress;
    VkAccelerationStructureBuildRangeInfoKHR* buildRanges = input->buildRanges.data();
    DeviceDispatch(vkCmdBuildAccelerationStructuresKHR(*commandBuffer, 1, &input->buildInfo, &buildRanges));
}

void AccelerationStructureBuilder::CreateBlas(CommandBuffer* commandBuffer,
                                              const std::vector<uint32_t> &indices,
                                              const std::vector<AccelerationStructureInput*>& inputs,
                                              VkDeviceAddress scratchAddress, QueryPool* queryPool) {
    if (queryPool) {
        vkResetQueryPool(*device, *queryPool, 0, static_cast<uint32_t>(indices.size()));
    }

    VkAccelerationStructureCreateFlagsKHR asCreateFlags = 0;
    if (compaction) {
        asCreateFlags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    }

    uint32_t queryCount = 0;

    for (const auto& index : indices) {
        AccelerationStructureInput* input = inputs[index];
        input->createFlags = asCreateFlags;

        // 1. create
        AccelerationStructure* as = new AccelerationStructure(input);

        // 2. build
        input->buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
        input->buildInfo.dstAccelerationStructure = *as;
        input->buildInfo.scratchData.deviceAddress = scratchAddress;
        VkAccelerationStructureBuildRangeInfoKHR* buildRanges = input->buildRanges.data();
        DeviceDispatch(vkCmdBuildAccelerationStructuresKHR(*commandBuffer, 1, &input->buildInfo, &buildRanges));

        // 3. buffer barrier
        VkMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        vkCmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1,
                             &barrier, 0, nullptr, 0, nullptr);

        // 4. update query pool
        if (queryPool) {
            VkAccelerationStructureKHR accelerationStructure = *as;
            DeviceDispatch(vkCmdWriteAccelerationStructuresPropertiesKHR(*commandBuffer,
                        1, &accelerationStructure, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
                        *queryPool, queryCount++));
        }

        // save this blas
        blas.push_back(as);
    }
}

void AccelerationStructureBuilder::CompactBlas(CommandBuffer* commandBuffer,
                                               const std::vector<uint32_t> &indices,
                                               const std::vector<AccelerationStructureInput*>& inputs,
                                               QueryPool* queryPool) {

    VkAccelerationStructureCreateFlagsKHR asCreateFlags = 0;

    // get the compacted size result back
    std::vector<VkDeviceSize> compactSizes(static_cast<uint32_t>(indices.size()));
    DeviceDispatch(vkGetQueryPoolResults(*device, *queryPool, 0,
            (uint32_t)compactSizes.size(), compactSizes.size() * sizeof(VkDeviceSize),
            compactSizes.data(), sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT));

    // compact blas size
    uint32_t queryCount = 0;

    for (const auto& index : indices) {
        AccelerationStructureInput* input = inputs[index];
        input->createFlags = asCreateFlags;
        input->sizeInfo.accelerationStructureSize = compactSizes[queryCount++];

        // 1. create
        AccelerationStructure* oldAs = blas[index];
        AccelerationStructure* newAs = new AccelerationStructure(input);

        // 2. copy
        VkCopyAccelerationStructureInfoKHR copyInfo = {};
        copyInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
        copyInfo.src = *oldAs;
        copyInfo.dst = *newAs;
        copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
        DeviceDispatch(vkCmdCopyAccelerationStructureKHR(*commandBuffer, &copyInfo));

        // 3. clean up (oldAs is automatically cleaned up)
        blas[index] = newAs;
    }
}
