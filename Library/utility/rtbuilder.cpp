#include "utility/rtbuilder.h"
#include "utility/scenegraph.h"

using namespace slim;

accel::Builder::Builder(Device* device) : device(device) {
}

void accel::Builder::EnableCompaction() {
    compaction = true;
}

void accel::Builder::AddNode(scene::Node* node, uint32_t sbtRecordOffset, uint32_t mask) {
    if (tlas.get() == nullptr) {
        VkAccelerationStructureCreateFlagsKHR createFlags = 0;
        tlas = new Instance(device, createFlags);
    }
    tlas->AddInstance(node, sbtRecordOffset, mask);
}

uint32_t accel::Builder::AddMesh(scene::Mesh* mesh) {
    VkAccelerationStructureCreateFlagsKHR createFlags = 0;
    Geometry* geometry = new Geometry(device, createFlags);

    // NOTE: assume position is in first vertex buffer, offset 0
    auto [vBuffer, vOffset] = mesh->GetVertexBuffer(0);
    auto [iBuffer, iOffset] = mesh->GetIndexBuffer();
    geometry->AddTriangles(iBuffer, iOffset, mesh->GetIndexCount(), mesh->GetIndexType(),
                           vBuffer, vOffset, mesh->GetVertexStride());

    uint32_t index = blas.size();
    blas.push_back(geometry);
    mesh->SetBlasGeometry(geometry);
    return index;
}

uint32_t accel::Builder::AddAABBs(Buffer* aabbsBuffer, uint32_t count, uint32_t stride) {
    VkAccelerationStructureCreateFlagsKHR createFlags = 0;
    Geometry* geometry = new Geometry(device, createFlags);
    geometry->AddAABBs(aabbsBuffer, count, stride);
    uint32_t index = blas.size();
    blas.push_back(geometry);
    return index;
}

void accel::Builder::BuildTlas() {
    // prepare acceleration structure input
    tlas->Prepare(false);   // build mode

    // find data structure sizes
    uint32_t maxScratchSize = tlas->sizeInfo.buildScratchSize;

    // scratch buffer
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    auto scratchBuffer = SlimPtr<Buffer>(device, maxScratchSize, bufferUsage, memoryUsage);
    VkDeviceAddress scratchAddress = device->GetDeviceAddress(scratchBuffer);

    // build tlas
    device->Execute([&](CommandBuffer* commandBuffer) {
        CreateTlas(commandBuffer, scratchAddress);
    });
}

void accel::Builder::BuildBlas() {
    // prepare acceleration structure input
    for (auto& as : blas) {
        as->Prepare(false); // build mode
    }

    // find data structure sizes
    uint32_t asTotalSize = 0;
    uint32_t maxScratchSize = 0;
    for (auto& as : blas) {
        uint32_t scratchSize = as->sizeInfo.buildScratchSize;
        maxScratchSize = std::max(maxScratchSize, scratchSize);
        asTotalSize += as->sizeInfo.accelerationStructureSize;
    }

    // scratch buffer
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    auto scratchBuffer = SlimPtr<Buffer>(device, maxScratchSize, bufferUsage, memoryUsage);
    VkDeviceAddress scratchAddress = device->GetDeviceAddress(scratchBuffer);

    // query pool
    SmartPtr<QueryPool> queryPool = nullptr;
    if (compaction) {
        queryPool = SlimPtr<QueryPool>(device, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, blas.size(), 0);
    }

    // create acceleration structures
    std::vector<uint32_t> indices;  // Indices of the BLAS to create
    VkDeviceSize batchSize{0};
    VkDeviceSize batchLimit{256'000'000};  // 256 MB
    for (uint32_t i = 0; i < blas.size(); i++) {
        indices.push_back(i);
        batchSize += blas[i]->sizeInfo.accelerationStructureSize;

        if (batchSize >= batchLimit || i == blas.size() - 1) {
            device->Execute([&](CommandBuffer* commandBuffer) {
                CreateBlas(commandBuffer, indices, scratchAddress, queryPool);
            });

            if (compaction) {
                device->Execute([&](CommandBuffer* commandBuffer) {
                    CompactBlas(commandBuffer, indices, queryPool);
                });
            }

            CleanNoncompacted(indices);

            batchSize = 0;
            indices.clear();
        } // end of batch size or end of inputs

    } // end of inputs for loop
}

void accel::Builder::CreateTlas(CommandBuffer* commandBuffer,
                                VkDeviceAddress scratchAddress) {
    // 1. create
    auto accel = new AccelStruct(tlas);
    tlas->accel = accel;

    // 2. build
    tlas->buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    tlas->buildInfo.dstAccelerationStructure = *accel;
    tlas->buildInfo.scratchData.deviceAddress = scratchAddress;
    VkAccelerationStructureBuildRangeInfoKHR* buildRanges = tlas->buildRanges.data();
    DeviceDispatch(vkCmdBuildAccelerationStructuresKHR(*commandBuffer, 1, &tlas->buildInfo, &buildRanges));

    // 3. buffer barrier
    VkMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    DeviceDispatch(vkCmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                                        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1,
                                        &barrier, 0, nullptr, 0, nullptr));
}

void accel::Builder::CreateBlas(CommandBuffer* commandBuffer,
                                const std::vector<uint32_t> &indices,
                                VkDeviceAddress scratchAddress, QueryPool* queryPool) {
    if (queryPool) {
        DeviceDispatch(vkResetQueryPool(*device, *queryPool, 0, static_cast<uint32_t>(indices.size())));
    }

    VkBuildAccelerationStructureFlagsKHR buildFlags = 0;
    if (compaction) {
        buildFlags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    }

    uint32_t queryCount = 0;

    for (const auto& index : indices) {
        auto& input = blas[index];
        input->buildInfo.flags |= buildFlags;

        // 1. create
        auto as = new AccelStruct(input);
        input->accel = as;

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
        DeviceDispatch(vkCmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                                            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1,
                                            &barrier, 0, nullptr, 0, nullptr));

        // 4. update query pool
        if (queryPool) {
            VkAccelerationStructureKHR accelerationStructure = *as;
            DeviceDispatch(vkCmdWriteAccelerationStructuresPropertiesKHR(*commandBuffer,
                        1, &accelerationStructure, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
                        *queryPool, queryCount++));
        }
    }
}

void accel::Builder::CompactBlas(CommandBuffer* commandBuffer,
                                 const std::vector<uint32_t> &indices,
                                 QueryPool* queryPool) {

    VkBuildAccelerationStructureFlagsKHR buildFlags = 0;

    // get the compacted size result back
    std::vector<VkDeviceSize> compactSizes(static_cast<uint32_t>(indices.size()));
    DeviceDispatch(vkGetQueryPoolResults(*device, *queryPool, 0,
            (uint32_t)compactSizes.size(), compactSizes.size() * sizeof(VkDeviceSize),
            compactSizes.data(), sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT));

    // compact blas size
    uint32_t queryCount = 0;

    for (const auto& index : indices) {
        auto& input = blas[index];
        input->buildInfo.flags |= buildFlags;
        input->sizeInfo.accelerationStructureSize = compactSizes[queryCount++];

        // 1. create
        input->cleanupAccel = input->accel;
        input->accel = new accel::AccelStruct(input);
        auto& oldAs = input->cleanupAccel;
        auto& newAs = input->accel;

        // 2. copy
        VkCopyAccelerationStructureInfoKHR copyInfo = {};
        copyInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
        copyInfo.src = *oldAs;
        copyInfo.dst = *newAs;
        copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
        DeviceDispatch(vkCmdCopyAccelerationStructureKHR(*commandBuffer, &copyInfo));
    }
}

void accel::Builder::CleanNoncompacted(const std::vector<uint32_t> &indices) {
    for (uint32_t index : indices) {
        blas[index]->cleanupAccel = nullptr;
    }
}
