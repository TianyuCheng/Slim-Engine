#include "surfel.h"

SurfelManager::SurfelManager(Device* device, uint32_t numSurfels) : device(device), numSurfels(numSurfels) {
    InitSurfelBuffer();
    InitSurfelDataBuffer();
    InitSurfelMomentBuffer();
    InitSurfelGridBuffer();
    InitSurfelCellBuffer();
    InitSurfelStatBuffer();
    InitSurfelIndirectBuffer();
}

void SurfelManager::InitSurfelBuffer() {
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    uint32_t bufferSize = numSurfels * sizeof(Surfel);
    surfelBuffer = SlimPtr<Buffer>(device, bufferSize, bufferUsage, memoryUsage);
    surfelBuffer->SetName("Surfels");
}

void SurfelManager::InitSurfelDataBuffer() {
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    uint32_t bufferSize = numSurfels * sizeof(SurfelData);
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
    uint32_t bufferSize = SURFEL_TABLE_SIZE * sizeof(uint32_t);
    surfelCellBuffer = SlimPtr<Buffer>(device, bufferSize, bufferUsage, memoryUsage);
    surfelCellBuffer->SetName("SurfelCell");
}

void SurfelManager::InitSurfelStatBuffer() {
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    uint32_t bufferSize = sizeof(SurfelStat);
    surfelStatBuffer = SlimPtr<Buffer>(device, bufferSize, bufferUsage, memoryUsage);
    surfelStatBuffer->SetName("SurfelStat");
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
        command.y = 0;
        command.z = 0;
        commandBuffer->CopyDataToBuffer(command, surfelIndirectBuffer);
    });
}

void AddSurfelPass(RenderGraph& renderGraph,
                   ResourceBundle& bundle,
                   Camera* camera,
                   GBuffer* gbuffer,
                   Visualize* visualize,
                   SurfelManager* surfel) {

    Device* device = renderGraph.GetRenderFrame()->GetDevice();

    struct FrameInfo {
        glm::uvec2 size;
    };

    struct CameraInfo {
        glm::mat4 invVP;
        glm::vec3 pos;
        float zNear;
        float zFar;
        float zFarRcp;
    };

    // sampler
    static auto sampler = bundle.AutoRelease(new Sampler(device, SamplerDesc {}));

    // surfel grid reset pipeline
    static auto surfelGridResetShader = bundle.AutoRelease(new spirv::ComputeShader(device, "main", "shaders/surfelgridreset.comp.spv"));
    static auto surfelGridResetPipeline = bundle.AutoRelease(new Pipeline(
        device,
        ComputePipelineDesc()
            .SetName("surfel-grid-reset")
            .SetComputeShader(surfelGridResetShader)
            .SetPipelineLayout(PipelineLayoutDesc()
                // set 0
                .AddBinding("SurfelGrid", SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            )
        )
    );

    // surfel grid offset pipeline
    static auto surfelGridOffsetShader = bundle.AutoRelease(new spirv::ComputeShader(device, "main", "shaders/surfelgridreset.comp.spv"));
    static auto surfelGridOffsetPipeline = bundle.AutoRelease(new Pipeline(
        device,
        ComputePipelineDesc()
            .SetName("surfel-grid-offset")
            .SetComputeShader(surfelGridOffsetShader)
            .SetPipelineLayout(PipelineLayoutDesc()
                // set 0
                .AddBinding("SurfelStat", SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                .AddBinding("SurfelGrid", SetBinding { 0, 1 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            )
        )
    );

    // surfel indirect prepare pipeline
    static auto surfelPrepareShader = bundle.AutoRelease(new spirv::ComputeShader(device, "main", "shaders/surfelprepare.comp.spv"));
    static auto surfelPreparePipeline = bundle.AutoRelease(new Pipeline(
        device,
        ComputePipelineDesc()
            .SetName("surfel-prepare")
            .SetComputeShader(surfelPrepareShader)
            .SetPipelineLayout(PipelineLayoutDesc()
                // set 0
                .AddBinding("SurfelStat",     SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                .AddBinding("SurfelIndirect", SetBinding { 0, 1 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            )
        )
    );

    // surfel update pipeline
    static Shader* surfelUpdateShader = bundle.AutoRelease(new spirv::ComputeShader(device, "main", "shaders/surfelupdate.comp.spv"));
    static auto surfelUpdatePipeline = bundle.AutoRelease(new Pipeline(
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
            )
        )
    );

    // surfel coverage pipeline
    static auto surfelCoverageShader = bundle.AutoRelease(new spirv::ComputeShader(device, "main", "shaders/surfelcov.comp.spv"));
    static auto surfelCoveragePipeline = bundle.AutoRelease(new Pipeline(
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
            )
        )
    );

    // ------------------------------------------------------------------------------------------------------------------

    // compile
    auto surfelComputePass = renderGraph.CreateComputePass("surfel-compute");
    surfelComputePass->SetTexture(gbuffer->depthBuffer);
    surfelComputePass->SetTexture(gbuffer->normalBuffer);
    surfelComputePass->SetTexture(gbuffer->objectBuffer);
    surfelComputePass->SetStorage(visualize->surfelcovBuffer, RenderGraph::STORAGE_IMAGE_WRITE_ONLY);

    // execute
    surfelComputePass->Execute([=](const RenderInfo& info) {
        // prepare camera data
        CameraInfo cameraData;
        cameraData.invVP = glm::inverse(camera->GetProjection() * camera->GetView());
        cameraData.pos = camera->GetPosition();
        cameraData.zNear = camera->GetNear();
        cameraData.zFar = camera->GetFar();
        cameraData.zFarRcp = 1.0 / camera->GetFar();
        auto cameraUniform = info.renderFrame->RequestUniformBuffer(cameraData);
        cameraUniform->SetName("Camera Uniform");

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
        }
        info.commandBuffer->EndRegion();

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
        }
        info.commandBuffer->EndRegion();

        // surfel update
        // This step copies surfel data to surfel
        // ---------------------------------------------------------------------------------------
        info.commandBuffer->BeginRegion("Surfel Update");
        {
            auto pipeline = surfelUpdatePipeline;
            info.commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetUniformBuffer("Camera", cameraUniform);
            descriptor->SetStorageBuffer("Surfel", surfel->surfelBuffer);
            descriptor->SetStorageBuffer("SurfelData", surfel->surfelDataBuffer);
            descriptor->SetStorageBuffer("SurfelGrid", surfel->surfelGridBuffer);
            descriptor->SetStorageBuffer("SurfelStat", surfel->surfelStatBuffer);
            info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

            // issue compute call
            vkCmdDispatchIndirect(*info.commandBuffer, *surfel->surfelIndirectBuffer, 0);
        }
        info.commandBuffer->EndRegion();

        // surfel coverage
        // This step spawns new surfels if needed.
        // Newly added surfels will be processed in the next frame.
        // ---------------------------------------------------------------------------------------
        info.commandBuffer->BeginRegion("Surfel Covereage");
        {
            auto pipeline = surfelCoveragePipeline;
            info.commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetUniformBuffer("Camera", cameraUniform);
            descriptor->SetStorageBuffer("Surfel", surfel->surfelBuffer);
            descriptor->SetStorageBuffer("SurfelData", surfel->surfelDataBuffer);
            descriptor->SetStorageBuffer("SurfelGrid", surfel->surfelGridBuffer);
            descriptor->SetStorageBuffer("SurfelCell", surfel->surfelCellBuffer);
            descriptor->SetStorageBuffer("SurfelStat", surfel->surfelStatBuffer);
            descriptor->SetTexture("Normal", gbuffer->normalBuffer->GetImage(), sampler);
            descriptor->SetTexture("Depth",  gbuffer->depthBuffer->GetImage(), sampler);
            descriptor->SetTexture("Object", gbuffer->objectBuffer->GetImage(), sampler);
            descriptor->SetStorageImage("Coverage", visualize->surfelcovBuffer->GetImage());
            info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

            // push constant
            FrameInfo frameInfo;
            frameInfo.size.x = info.renderFrame->GetExtent().width;
            frameInfo.size.y = info.renderFrame->GetExtent().height;
            info.commandBuffer->PushConstants(pipeline->Layout(), "Info", &frameInfo);

            // issue compute call
            const auto& extent = info.renderFrame->GetExtent();
            uint32_t groupCountX = extent.width / SURFEL_TILE_X;
            uint32_t groupCountY = extent.width / SURFEL_TILE_Y;
            uint32_t groupCountZ = 1;
            info.commandBuffer->Dispatch(groupCountX, groupCountY, groupCountZ);
        }
        info.commandBuffer->EndRegion();
    });
}
