#include "surfel.h"

SurfelManager::SurfelManager(Device* device) : device(device) {
    InitSurfelBuffer();
    InitSurfelDataBuffer();
    InitSurfelMomentBuffer();
    InitSurfelGridBuffer();
    InitSurfelCellBuffer();
    InitSurfelStatBuffer();
    InitSurfelIndirectBuffer();
    #ifdef ENABLE_RAY_TRACING
    InitSurfelAabbBuffer();
    InitSurfelAabbAccelStructure();
    #endif
}

void SurfelManager::InitSurfelBuffer() {
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    uint32_t bufferSize = SURFEL_CAPACITY * sizeof(Surfel);
    surfelBuffer = SlimPtr<Buffer>(device, bufferSize, bufferUsage, memoryUsage);
    surfelBuffer->SetName("Surfels");
}

void SurfelManager::InitSurfelDataBuffer() {
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    uint32_t bufferSize = SURFEL_CAPACITY * sizeof(SurfelData);
    surfelDataBuffer = SlimPtr<Buffer>(device, bufferSize, bufferUsage, memoryUsage);
    surfelDataBuffer->SetName("SurfelData");
}

void SurfelManager::InitSurfelMomentBuffer() {
    // do nothing now
}

void SurfelManager::InitSurfelGridBuffer() {
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    uint32_t bufferSize = SURFEL_TABLE_SIZE * sizeof(SurfelGrid);
    surfelGridBuffer = SlimPtr<Buffer>(device, bufferSize, bufferUsage, memoryUsage);
    surfelGridBuffer->SetName("SurfelGrid");
}

void SurfelManager::InitSurfelCellBuffer() {
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    uint32_t bufferSize = SURFEL_TABLE_SIZE * sizeof(uint32_t) * 64;
    surfelCellBuffer = SlimPtr<Buffer>(device, bufferSize, bufferUsage, memoryUsage);
    surfelCellBuffer->SetName("SurfelCell");
}

void SurfelManager::InitSurfelAabbBuffer() {
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                                   | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                                   | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    uint32_t bufferSize = sizeof(VkAabbPositionsKHR) * SURFEL_CAPACITY;
    surfelAabbBuffer = SlimPtr<Buffer>(device, bufferSize, bufferUsage, memoryUsage);
    surfelAabbBuffer->SetName("SurfelAabb");
    device->Execute([&](CommandBuffer* commandBuffer) {
        std::vector<VkAabbPositionsKHR> aabbs(SURFEL_CAPACITY);
        for (uint32_t i = 0; i < SURFEL_CAPACITY; i++) {
            aabbs[i].minX = 0.0;
            aabbs[i].minY = 0.0;
            aabbs[i].minZ = 0.0;
            aabbs[i].maxX = 0.0;
            aabbs[i].maxY = 0.0;
            aabbs[i].maxZ = 0.0;
        }
        commandBuffer->CopyDataToBuffer(aabbs, surfelAabbBuffer);
    });
}

void SurfelManager::InitSurfelStatBuffer() {
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    uint32_t bufferSize = sizeof(SurfelStat);
    surfelStatBuffer = SlimPtr<Buffer>(device, bufferSize, bufferUsage, memoryUsage);
    surfelStatBuffer->SetName("SurfelStat");
    device->Execute([&](CommandBuffer* commandBuffer) {
        SurfelStat stat;
        stat.count = 0;
        stat.cellAllocator = 0;
        stat.debug0 = 0;
        stat.debug1 = 0;
        commandBuffer->CopyDataToBuffer(stat, surfelStatBuffer);
    });

    surfelStatBufferCPU = SlimPtr<Buffer>(device, bufferSize, bufferUsage, VMA_MEMORY_USAGE_CPU_ONLY);
    surfelStatBufferCPU->SetName("SurfelStatCPU");
}

void SurfelManager::InitSurfelIndirectBuffer() {
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    uint32_t bufferSize = sizeof(VkDispatchIndirectCommand);
    surfelIndirectBuffer = SlimPtr<Buffer>(device, bufferSize, bufferUsage, memoryUsage);
    surfelIndirectBuffer->SetName("SurfelIndirect");
    device->Execute([&](CommandBuffer* commandBuffer) {
        VkDispatchIndirectCommand command;
        command.x = 0;
        command.y = 1;
        command.z = 1;
        commandBuffer->CopyDataToBuffer(command, surfelIndirectBuffer);
    });
}

void SurfelManager::InitSurfelAabbAccelStructure() {
    // blas
    accelBuilder = SlimPtr<accel::Builder>(device);
    accelBuilder->AddAABBs(surfelAabbBuffer, SURFEL_CAPACITY, sizeof(VkAabbPositionsKHR));
    accelBuilder->BuildBlas();

    // mesh
    aabbMesh = SlimPtr<scene::Mesh>();
    aabbMesh->SetBlasGeometry(accelBuilder->GetBlasGeometry(0));

    // node
    aabbNode = SlimPtr<scene::Node>();
    aabbNode->SetDraw(aabbMesh, nullptr);

    // tlas, use hit group 2 for surfel
    accelBuilder->AddNode(aabbNode, 1, 0x2);
    accelBuilder->BuildTlas();
}

void SurfelManager::UpdateAABB() const {
    device->WaitIdle();
    accelBuilder->BuildBlas();
}

void AddSurfelPass(RenderGraph& renderGraph,
                   AutoReleasePool& pool,
                   MainScene* scene,
                   GBuffer* gbuffer,
                   Visualize* visualize,
                   SurfelManager* surfel,
                   uint32_t frameId) {

    struct FrameInfo {
        glm::uvec2 size;
        uint32_t frameId;
    };

    // sampler
    static auto sampler = pool.FetchOrCreate(
        "nearest.sampler",
        [](Device* device) {
            return new Sampler(device,
                SamplerDesc {}
                    .MinFilter(VK_FILTER_NEAREST)
                    .MagFilter(VK_FILTER_NEAREST));
        });

    // surfel indirect prepare pipeline
    static auto surfelPrepareShader = pool.FetchOrCreate(
        "surfel.prepare.shader",
        [](Device* device) {
            return new spirv::ComputeShader(device, "main", "shaders/surfelprepare.comp.spv");
        });
    static auto surfelPreparePipeline = pool.FetchOrCreate(
        "surfel.prepare.pipeline",
        [&](Device* device) {
            return new Pipeline(
                device,
                ComputePipelineDesc()
                .SetName("surfel-prepare")
                .SetComputeShader(surfelPrepareShader)
                .SetPipelineLayout(PipelineLayoutDesc()
                    // set 0
                    .AddBinding("SurfelStat",     SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("SurfelIndirect", SetBinding { 0, 1 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    )
                );
        }
    );

    // surfel grid reset pipeline
    static auto surfelGridResetShader = pool.FetchOrCreate(
        "surfel.grid.reset.shader",
        [&](Device* device) {
            return new spirv::ComputeShader(device, "main", "shaders/surfelgridreset.comp.spv");
        });
    static auto surfelGridResetPipeline = pool.FetchOrCreate(
        "surfel.grid.reset.pipeline",
        [&](Device* device) {
            return new Pipeline(
                device,
                ComputePipelineDesc()
                    .SetName("surfel-grid-reset")
                    .SetComputeShader(surfelGridResetShader)
                    .SetPipelineLayout(PipelineLayoutDesc()
                        // set 0
                        .AddBinding("SurfelGrid", SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    )
                );
        }
    );

    // surfel update pipeline
    static auto surfelUpdateShader = pool.FetchOrCreate(
        "surfel.update.shader",
        [&](Device* device) {
            return new spirv::ComputeShader(device, "main", "shaders/surfelupdate.comp.spv");
        });
    static auto surfelUpdatePipeline = pool.FetchOrCreate(
        "surfel.update.shader",
        [&](Device* device) {
            return new Pipeline(
                device,
                ComputePipelineDesc()
                    .SetName("surfel-update")
                    .SetComputeShader(surfelUpdateShader)
                    .SetPipelineLayout(PipelineLayoutDesc()
                        // set 0
                        .AddBinding("Camera",     SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                        // set 1
                        .AddBinding("Surfel",     SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                        .AddBinding("SurfelData", SetBinding { 1, 1 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                        .AddBinding("SurfelGrid", SetBinding { 1, 2 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                        .AddBinding("SurfelStat", SetBinding { 1, 3 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                        .AddBinding("SurfelAabb", SetBinding { 1, 4 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    )
                );
        }
    );

    // surfel grid offset pipeline
    static auto surfelGridOffsetShader = pool.FetchOrCreate(
        "surfel.grid.offset.shader",
        [&](Device* device) {
            return new spirv::ComputeShader(device, "main", "shaders/surfelgridoffset.comp.spv");
        });
    static auto surfelGridOffsetPipeline = pool.FetchOrCreate(
        "surfel.grid.offset.pipeline",
        [&](Device* device) {
            return new Pipeline(
                device,
                ComputePipelineDesc()
                    .SetName("surfel-grid-offset")
                    .SetComputeShader(surfelGridOffsetShader)
                    .SetPipelineLayout(PipelineLayoutDesc()
                        // set 0
                        .AddBinding("SurfelStat", SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                        .AddBinding("SurfelGrid", SetBinding { 0, 1 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    )
                );
        }
    );

    // surfel grid binning pipeline
    static auto surfelBinningShader = pool.FetchOrCreate(
        "surfel.binning.shader",
        [&](Device* device) {
            return new spirv::ComputeShader(device, "main", "shaders/surfelbinning.comp.spv");
        });
    static auto surfelBinningPipeline = pool.FetchOrCreate(
        "surfel.binning.pipeline",
        [&](Device* device) {
            return new Pipeline(
                device,
                ComputePipelineDesc()
                    .SetName("surfel-binning")
                    .SetComputeShader(surfelBinningShader)
                    .SetPipelineLayout(PipelineLayoutDesc()
                        // set 0
                        .AddBinding("Camera",     SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                        // set 1
                        .AddBinding("SurfelStat", SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                        .AddBinding("Surfel",     SetBinding { 1, 1 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                        .AddBinding("SurfelGrid", SetBinding { 1, 2 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                        .AddBinding("SurfelCell", SetBinding { 1, 3 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    )
                );
        }
    );

    // surfel coverage pipeline
    static auto surfelCoverageShader = pool.FetchOrCreate(
        "surfel.coverage.shader",
        [&](Device* device) {
            return new spirv::ComputeShader(device, "main", "shaders/surfelcov.comp.spv");
        });
    static auto surfelCoveragePipeline = pool.FetchOrCreate(
        "surfel.coverage.pipeline",
        [&](Device* device) {
            return new Pipeline(
                device,
                ComputePipelineDesc()
                    .SetName("surfel-coverage")
                    .SetComputeShader(surfelCoverageShader)
                    .SetPipelineLayout(PipelineLayoutDesc()
                        .AddPushConstant("Info", Range { 0, sizeof(FrameInfo) }, VK_SHADER_STAGE_COMPUTE_BIT)
                        // set 0
                        .AddBinding("Camera",     SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                        // set 1
                        .AddBinding("Surfel",     SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                        .AddBinding("SurfelData", SetBinding { 1, 1 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                        .AddBinding("SurfelGrid", SetBinding { 1, 2 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                        .AddBinding("SurfelCell", SetBinding { 1, 3 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                        .AddBinding("SurfelStat", SetBinding { 1, 4 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                        // set 2
                        .AddBinding("Depth",      SetBinding { 2, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
                        .AddBinding("Normal",     SetBinding { 2, 1 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
                        .AddBinding("Object",     SetBinding { 2, 2 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
                        .AddBinding("Coverage",   SetBinding { 2, 3 }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_SHADER_STAGE_COMPUTE_BIT)
                        .AddBinding("Debug",      SetBinding { 2, 4 }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_SHADER_STAGE_COMPUTE_BIT)
                    )
                );
        }
    );

    #ifdef ENABLE_RAY_TRACING
    // ray gen shader
    static auto rayGenShader = pool.FetchOrCreate(
        "raytrace.ray.gen",
        [](Device* device) {
            return new spirv::RayGenShader(device, "main", "shaders/raytrace.rgen.spv");
        });
    // shadow ray miss shader
    static auto shadowMissShader = pool.FetchOrCreate(
        "shadow.ray.miss",
        [](Device* device) {
            return new spirv::MissShader(device, "main", "shaders/shadow.rmiss.spv");
        });
    // shadow ray closest hit shader
    static auto shadowClosestHitShader = pool.FetchOrCreate(
        "shadow.ray.closest.hit",
        [](Device* device) {
            return new spirv::ClosestHitShader(device, "main", "shaders/shadow.rchit.spv");
        });
    // surfel ray miss shader
    static auto surfelMissShader = pool.FetchOrCreate(
        "surel.ray.miss",
        [](Device* device) {
            return new spirv::MissShader(device, "main", "shaders/surfel.rmiss.spv");
        });
    // surfel ray closest hit shader
    static auto surfelClosestHitShader = pool.FetchOrCreate(
        "surfel.ray.closest.hit",
        [](Device* device) {
            return new spirv::ClosestHitShader(device, "main", "shaders/surfel.rchit.spv");
        });
    // surfel ray intersection shader
    static auto surfelIntersectionShader = pool.FetchOrCreate(
        "surel.ray.intersection",
        [](Device* device) {
            return new spirv::IntersectionShader(device, "main", "shaders/surfel.rint.spv");
        });
    // ray tracing pipeline
    static auto raytracePipeline = pool.FetchOrCreate(
        "surfel.raytrace.pipline",
        [&](Device* device) {
            return new Pipeline(
                device,
                RayTracingPipelineDesc()
                    .SetName("surfel-ray-tracing")
                    .SetRayGenShader(rayGenShader)
                    // ---------------------------------------
                    .SetMissShader(shadowMissShader)
                    .SetClosestHitShader(shadowClosestHitShader)
                    // ---------------------------------------
                    .SetMissShader(surfelMissShader)
                    .SetClosestHitShader(surfelClosestHitShader, surfelIntersectionShader)
                    // ---------------------------------------
                    .SetMaxRayRecursionDepth(1)
                    .SetPipelineLayout(PipelineLayoutDesc()
                        .AddPushConstant("FrameInfo", Range { 0, sizeof(uint32_t) }, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Scene Accel",  SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Surfel Accel", SetBinding { 0, 1 }, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Camera",       SetBinding { 0, 2 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Light",        SetBinding { 0, 3 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("SurfelStat",   SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR)
                        .AddBinding("SurfelData",   SetBinding { 1, 1 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR)
                        .AddBinding("Surfel",       SetBinding { 1, 2 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR)
                    )
                );
        }
    );
    #endif

    // ------------------------------------------------------------------------------------------------------------------

    // compile
    auto surfelComputePass = renderGraph.CreateComputePass("surfel-compute");
    surfelComputePass->SetTexture(gbuffer->depthBuffer);
    surfelComputePass->SetTexture(gbuffer->normalBuffer);
    surfelComputePass->SetTexture(gbuffer->objectBuffer);
    surfelComputePass->SetStorage(surfel->surfelCovBuffer, RenderGraph::STORAGE_WRITE_ONLY);
    surfelComputePass->SetStorage(visualize->surfelCovBuffer, RenderGraph::STORAGE_WRITE_ONLY);
    surfelComputePass->SetStorage(scene->cameraResource, RenderGraph::STORAGE_READ_ONLY);
    surfelComputePass->SetStorage(scene->lightResource, RenderGraph::STORAGE_READ_ONLY);

    // execute
    surfelComputePass->Execute([=](const RenderInfo& info) {

        // surfel indirect prepare
        // This step update the indirect draw buffer's dispatch count based on active surfel count
        // ---------------------------------------------------------------------------------------
        info.commandBuffer->BeginRegion("Surfel Indirect Prepare");
        {
            auto pipeline = surfelPreparePipeline;
            info.commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetStorageBuffer("SurfelStat",     surfel->surfelStatBuffer);
            descriptor->SetStorageBuffer("SurfelIndirect", surfel->surfelIndirectBuffer);
            info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

            // issue compute call
            info.commandBuffer->Dispatch(1, 1, 1);

            // barriers
            info.commandBuffer->PrepareForBuffer(surfel->surfelIndirectBuffer,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
        info.commandBuffer->EndRegion();

        // surfel grid reset
        // This step reset grid's count.
        // ---------------------------------------------------------------------------------------
        info.commandBuffer->BeginRegion("Surfel Grid Reset");
        {
            auto pipeline = surfelGridResetPipeline;
            info.commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetStorageBuffer("SurfelGrid", surfel->surfelGridBuffer);
            info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

            // issue compute call
            uint32_t groupCountX = SURFEL_TABLE_SIZE / SURFEL_UPDATE_GROUP_SIZE;
            uint32_t groupCountY = 1;
            uint32_t groupCountZ = 1;
            info.commandBuffer->Dispatch(groupCountX, groupCountY, groupCountZ);

            // barriers
            info.commandBuffer->PrepareForBuffer(surfel->surfelGridBuffer,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
        info.commandBuffer->EndRegion();

        // surfel update
        // This step copies surfel data to surfel, and accumulate surfel grid count
        // ---------------------------------------------------------------------------------------
        info.commandBuffer->BeginRegion("Surfel Update");
        {
            auto pipeline = surfelUpdatePipeline;
            info.commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetUniformBuffer("Camera", scene->cameraBuffer);
            descriptor->SetStorageBuffer("Surfel", surfel->surfelBuffer);
            descriptor->SetStorageBuffer("SurfelData", surfel->surfelDataBuffer);
            descriptor->SetStorageBuffer("SurfelGrid", surfel->surfelGridBuffer);
            descriptor->SetStorageBuffer("SurfelStat", surfel->surfelStatBuffer);
            descriptor->SetStorageBuffer("SurfelAabb", surfel->surfelAabbBuffer);
            info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

            // issue compute call
            vkCmdDispatchIndirect(*info.commandBuffer, *surfel->surfelIndirectBuffer, 0);

            // barriers
            info.commandBuffer->PrepareForBuffer(surfel->surfelBuffer,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            info.commandBuffer->PrepareForBuffer(surfel->surfelGridBuffer,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            info.commandBuffer->PrepareForBuffer(surfel->surfelAabbBuffer,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
        info.commandBuffer->EndRegion();

        // surfel grid offset
        // This step accumulates grids' global offset with cell allocator atomics.
        // ---------------------------------------------------------------------------------------
        info.commandBuffer->BeginRegion("Surfel Grid Offset");
        {
            auto pipeline = surfelGridOffsetPipeline;
            info.commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetStorageBuffer("SurfelStat", surfel->surfelStatBuffer);
            descriptor->SetStorageBuffer("SurfelGrid", surfel->surfelGridBuffer);
            info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

            // issue compute call
            uint32_t groupCountX = SURFEL_TABLE_SIZE / SURFEL_UPDATE_GROUP_SIZE;
            uint32_t groupCountY = 1;
            uint32_t groupCountZ = 1;
            info.commandBuffer->Dispatch(groupCountX, groupCountY, groupCountZ);

            // barriers
            info.commandBuffer->PrepareForBuffer(surfel->surfelGridBuffer,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
        info.commandBuffer->EndRegion();

        // surfel grid reset
        // This step reset grid's count.
        // ---------------------------------------------------------------------------------------
        info.commandBuffer->BeginRegion("Surfel Grid Reset v2");
        {
            auto pipeline = surfelGridResetPipeline;
            info.commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetStorageBuffer("SurfelGrid", surfel->surfelGridBuffer);
            info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

            // issue compute call
            uint32_t groupCountX = SURFEL_TABLE_SIZE / SURFEL_UPDATE_GROUP_SIZE;
            uint32_t groupCountY = 1;
            uint32_t groupCountZ = 1;
            info.commandBuffer->Dispatch(groupCountX, groupCountY, groupCountZ);

            // barriers
            info.commandBuffer->PrepareForBuffer(surfel->surfelGridBuffer,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
        info.commandBuffer->EndRegion();

        // surfel binning
        // This step puts surfel cell index into correct place
        // ---------------------------------------------------------------------------------------
        info.commandBuffer->BeginRegion("Surfel Binning");
        {
            auto pipeline = surfelBinningPipeline;
            info.commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetUniformBuffer("Camera", scene->cameraBuffer);
            descriptor->SetStorageBuffer("Surfel", surfel->surfelBuffer);
            descriptor->SetStorageBuffer("SurfelGrid", surfel->surfelGridBuffer);
            descriptor->SetStorageBuffer("SurfelCell", surfel->surfelCellBuffer);
            descriptor->SetStorageBuffer("SurfelStat", surfel->surfelStatBuffer);
            info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

            // issue compute call
            vkCmdDispatchIndirect(*info.commandBuffer, *surfel->surfelIndirectBuffer, 0);

            // barriers
            info.commandBuffer->PrepareForBuffer(surfel->surfelCellBuffer,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
        info.commandBuffer->EndRegion();

        // surfel coverage
        // This step spawns new surfels if needed.
        // Newly added surfels will be processed in the next frame.
        // ---------------------------------------------------------------------------------------
        info.commandBuffer->BeginRegion("Surfel Coverage");
        {

            auto pipeline = surfelCoveragePipeline;
            info.commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetUniformBuffer("Camera", scene->cameraBuffer);
            descriptor->SetStorageBuffer("Surfel", surfel->surfelBuffer);
            descriptor->SetStorageBuffer("SurfelData", surfel->surfelDataBuffer);
            descriptor->SetStorageBuffer("SurfelGrid", surfel->surfelGridBuffer);
            descriptor->SetStorageBuffer("SurfelCell", surfel->surfelCellBuffer);
            descriptor->SetStorageBuffer("SurfelStat", surfel->surfelStatBuffer);
            descriptor->SetTexture("Normal", gbuffer->normalBuffer->GetImage(), sampler);
            descriptor->SetTexture("Depth",  gbuffer->depthBuffer->GetImage(), sampler);
            descriptor->SetTexture("Object", gbuffer->objectBuffer->GetImage(), sampler);
            descriptor->SetStorageImage("Coverage", surfel->surfelCovBuffer->GetImage());
            descriptor->SetStorageImage("Debug", visualize->surfelCovBuffer->GetImage());
            info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

            // push constant
            FrameInfo frameInfo;
            frameInfo.size.x = info.renderFrame->GetExtent().width;
            frameInfo.size.y = info.renderFrame->GetExtent().height;
            frameInfo.frameId = frameId;
            info.commandBuffer->PushConstants(pipeline->Layout(), "Info", &frameInfo);

            // issue compute call
            const auto& extent = info.renderFrame->GetExtent();
            uint32_t groupCountX = extent.width / SURFEL_TILE_X;
            uint32_t groupCountY = extent.width / SURFEL_TILE_Y;
            uint32_t groupCountZ = 1;
            info.commandBuffer->Dispatch(groupCountX, groupCountY, groupCountZ);

            // barriers
            info.commandBuffer->PrepareForBuffer(surfel->surfelDataBuffer,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            info.commandBuffer->PrepareForBuffer(surfel->surfelStatBuffer,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
        info.commandBuffer->EndRegion();

        #ifdef ENABLE_RAY_TRACING
        // surfel ray query
        // This step spawns new surfels if needed.
        // Newly added surfels will be processed in the next frame.
        // ---------------------------------------------------------------------------------------
        info.commandBuffer->BeginRegion("Surfel Ray Query");
        {
            auto pipeline = raytracePipeline;

            // bind pipeline
            info.commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetAccelStruct("Scene Accel", scene->GetTlas());
            descriptor->SetAccelStruct("Surfel Accel", surfel->GetTlas());
            descriptor->SetUniformBuffer("Camera", scene->cameraBuffer);
            descriptor->SetUniformBuffer("Light", scene->lightBuffer);
            descriptor->SetStorageBuffer("SurfelStat", surfel->surfelStatBuffer);
            descriptor->SetStorageBuffer("SurfelData", surfel->surfelDataBuffer);
            descriptor->SetStorageBuffer("Surfel",     surfel->surfelBuffer);
            info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());
            info.commandBuffer->PushConstants(pipeline->Layout(), "FrameInfo", &frameId);

            // trace rays
            vkCmdTraceRaysKHR(*info.commandBuffer,
                              pipeline->GetRayGenRegion(),
                              pipeline->GetMissRegion(),
                              pipeline->GetHitRegion(),
                              pipeline->GetCallableRegion(),
                              SURFEL_CAPACITY_SQRT, SURFEL_CAPACITY_SQRT, 1);

            info.commandBuffer->PrepareForBuffer(surfel->surfelDataBuffer,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
        info.commandBuffer->EndRegion();
        #endif
    });
}
