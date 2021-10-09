#include "raytrace.h"

void AddRayTracePass(RenderGraph& renderGraph,
                     ResourceBundle& bundle,
                     MainScene* scene,
                     GBuffer* gbuffer,
                     accel::AccelStruct* tlas) {

    Device* device = renderGraph.GetRenderFrame()->GetDevice();

    // ray gen shader
    static Shader* shadowRayGenShader = bundle.AutoRelease(new spirv::RayGenShader(device, "main", "shaders/shadow.rgen.spv"));

    // shadow ray miss shader
    static Shader* shadowRayMissShader = bundle.AutoRelease(new spirv::MissShader(device, "main", "shaders/shadow.rmiss.spv"));

    // shadow ray closest hit shader
    static Shader* shadowRayClosestHitShader = bundle.AutoRelease(new spirv::ClosestHitShader(device, "main", "shaders/shadow.rchit.spv"));

    // ray tracing pipeline
    static Pipeline* pipeline = bundle.AutoRelease(
        new Pipeline(
            device,
            RayTracingPipelineDesc()
                .SetName("hybrid-raytracing")
                .SetRayGenShader(shadowRayGenShader)
                .SetMissShader(shadowRayMissShader)
                .SetClosestHitShader(shadowRayClosestHitShader)
                .SetMaxRayRecursionDepth(1)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("Accel",    SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                    .AddBinding("Albedo",   SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                    .AddBinding("Normal",   SetBinding { 1, 1 }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                    .AddBinding("Position", SetBinding { 1, 2 }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                    .AddBinding("Light",    SetBinding { 2, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                )
        )
    );

    // hybrid ray tracer / rasterizer
    auto raytracePass = renderGraph.CreateComputePass("raytrace");
    raytracePass->SetStorage(gbuffer->albedoBuffer);
    raytracePass->SetStorage(gbuffer->normalBuffer);
    raytracePass->SetStorage(gbuffer->positionBuffer);
    raytracePass->Execute([=](const RenderInfo& info) {
        CommandBuffer* commandBuffer = info.commandBuffer;
        RenderFrame* renderFrame = info.renderFrame;

        // bind pipeline
        commandBuffer->BindPipeline(pipeline);

        // bind descriptor
        auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetAccelStruct("Accel", tlas);
        descriptor->SetStorageImage("Albedo", gbuffer->albedoBuffer->GetImage());
        descriptor->SetStorageImage("Normal", gbuffer->normalBuffer->GetImage());
        descriptor->SetStorageImage("Position", gbuffer->positionBuffer->GetImage());
        descriptor->SetUniformBuffer("Light", scene->lightBuffer);
        commandBuffer->BindDescriptor(descriptor, pipeline->Type());

        // trace rays
        const VkExtent3D& extent = gbuffer->albedoBuffer->GetImage()->GetExtent();
        vkCmdTraceRaysKHR(*commandBuffer,
                          pipeline->GetRayGenRegion(),
                          pipeline->GetMissRegion(),
                          pipeline->GetHitRegion(),
                          pipeline->GetCallableRegion(),
                          extent.width, extent.height, 1);
    });
}
