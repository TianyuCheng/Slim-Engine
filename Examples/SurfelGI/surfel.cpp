#include "surfel.h"

SurfelManager::SurfelManager(Device* device, uint32_t numSurfels) : device(device), numSurfels(numSurfels) {
    InitSurfelBuffer();
    InitSurfelDataBuffer();
    InitSurfelMomentBuffer();
    InitSurfelGridBuffer();
    InitSurfelCellBuffer();
    InitSurfelStatBuffer();
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

void AddSurfelCovPass(RenderGraph& renderGraph,
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
    };

    // sampler
    static Sampler* sampler = bundle.AutoRelease(new Sampler(device, SamplerDesc {}));

    // compute shader
    static Shader* cShader = bundle.AutoRelease(new spirv::ComputeShader(device, "main", "shaders/surfelcov.comp.spv"));

    // pipeline
    static auto pipeline = bundle.AutoRelease(new Pipeline(
        device,
        ComputePipelineDesc()
            .SetName("surfelcov")
            .SetComputeShader(cShader)
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
    auto surfelCovPass = renderGraph.CreateComputePass("surfelcov");
    surfelCovPass->SetTexture(gbuffer->depthBuffer);
    surfelCovPass->SetTexture(gbuffer->normalBuffer);
    surfelCovPass->SetTexture(gbuffer->objectBuffer);
    surfelCovPass->SetStorage(visualize->surfelcovBuffer, RenderGraph::STORAGE_IMAGE_WRITE_ONLY);

    // execute
    surfelCovPass->Execute([=](const RenderInfo& info) {
        info.commandBuffer->BindPipeline(pipeline);

        // prepare camera data
        CameraInfo cameraData;
        cameraData.invVP = glm::inverse(camera->GetProjection() * camera->GetView());
        cameraData.pos = camera->GetPosition();
        cameraData.zNear = camera->GetNear();
        cameraData.zFar = camera->GetFar();

        // bind descriptor
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetUniformBuffer("Camera", info.renderFrame->RequestUniformBuffer(cameraData));
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
        uint32_t groupCountX = extent.width / TILE_X;
        uint32_t groupCountY = extent.width / TILE_Y;
        uint32_t groupCountZ = 1;
        info.commandBuffer->Dispatch(groupCountX, groupCountY, groupCountZ);
    });
}
