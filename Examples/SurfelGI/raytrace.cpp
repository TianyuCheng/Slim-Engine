#include "raytrace.h"

void AddRayTracePass(RenderGraph& renderGraph,
                     AutoReleasePool& pool,
                     GBuffer* gbuffer,
                     RayTrace* raytrace,
                     accel::AccelStruct* tlas,
                     Camera* camera,
                     DirectionalLight* light) {

    struct FrameInfo {
        glm::vec4 lightDirection;
        glm::vec4 lightColor;
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
    static auto sampler = pool.FetchOrCreate(
        "nearest.sampler",
        [](Device* device) {
            return new Sampler(device,
                SamplerDesc {}
                    .MinFilter(VK_FILTER_NEAREST)
                    .MagFilter(VK_FILTER_NEAREST));
        });

    // ray gen shader
    static auto shadowRayGenShader = pool.FetchOrCreate(
        "shadow.ray.gen",
        [](Device* device) {
            return new spirv::RayGenShader(device, "main", "shaders/shadow.rgen.spv");
        }
    );

    // shadow ray miss shader
    static auto shadowRayMissShader = pool.FetchOrCreate(
        "shadow.ray.miss",
        [](Device* device) {
            return new spirv::MissShader(device, "main", "shaders/shadow.rmiss.spv");
        }
    );

    // shadow ray closest hit shader
    static auto shadowRayClosestHitShader = pool.FetchOrCreate(
        "shadow.ray.closest.hit",
        [](Device* device) {
            return new spirv::ClosestHitShader(device, "main", "shaders/shadow.rchit.spv");
        }
    );

    // ray tracing pipeline
    static auto shadowPipeline = pool.FetchOrCreate(
        "shadow.raytrace.pipline",
        [&](Device* device) {
            return new Pipeline(
                device,
                RayTracingPipelineDesc()
                    .SetName("hybrid-raytracing")
                    .SetRayGenShader(shadowRayGenShader)
                    .SetMissShader(shadowRayMissShader)
                    .SetClosestHitShader(shadowRayClosestHitShader)
                    .SetMaxRayRecursionDepth(1)
                    .SetPipelineLayout(PipelineLayoutDesc()
                        .AddPushConstant("FrameInfo", Range { 0, sizeof(FrameInfo) }, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Camera", SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Accel",  SetBinding { 0, 1 }, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Depth",  SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Normal", SetBinding { 1, 1 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Shadow", SetBinding { 2, 0 }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                    )
                );
        }
    );

    // hybrid ray tracer / rasterizer
    auto raytracePass = renderGraph.CreateComputePass("raytrace");
    raytracePass->SetTexture(gbuffer->depthBuffer);
    raytracePass->SetTexture(gbuffer->normalBuffer);
    raytracePass->SetStorage(raytrace->shadowBuffer, RenderGraph::STORAGE_IMAGE_WRITE_ONLY); // shadow information
    raytracePass->SetStorage(raytrace->surfelBuffer, RenderGraph::STORAGE_IMAGE_WRITE_ONLY); // surfel information
    raytracePass->Execute([=](const RenderInfo& info) {
        CommandBuffer* commandBuffer = info.commandBuffer;
        RenderFrame* renderFrame = info.renderFrame;

        // preapre frame info
        FrameInfo frameInfo;
        frameInfo.lightColor = light->color;
        frameInfo.lightDirection = light->direction;
        frameInfo.size.x = info.renderFrame->GetExtent().width;
        frameInfo.size.y = info.renderFrame->GetExtent().height;

        // prepare camera data
        CameraInfo cameraData;
        cameraData.invVP = glm::inverse(camera->GetProjection() * camera->GetView());
        cameraData.pos = camera->GetPosition();
        cameraData.zNear = camera->GetNear();
        cameraData.zFar = camera->GetFar();
        cameraData.zFarRcp = 1.0 / camera->GetFar();
        auto cameraUniform = info.renderFrame->RequestUniformBuffer(cameraData);
        cameraUniform->SetName("Camera Uniform");

        // shadow ray query
        {
            auto pipeline = shadowPipeline;

            // bind pipeline
            commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetUniformBuffer("Camera", cameraUniform);
            descriptor->SetAccelStruct("Accel", tlas);
            descriptor->SetTexture("Depth", gbuffer->depthBuffer->GetImage(), sampler);
            descriptor->SetTexture("Normal", gbuffer->normalBuffer->GetImage(), sampler);
            descriptor->SetStorageImage("Shadow", raytrace->shadowBuffer->GetImage());
            commandBuffer->BindDescriptor(descriptor, pipeline->Type());
            commandBuffer->PushConstants(pipeline->Layout(), "Info", &frameInfo);

            // trace rays
            const VkExtent3D& extent = gbuffer->albedoBuffer->GetImage()->GetExtent();
            vkCmdTraceRaysKHR(*commandBuffer,
                              pipeline->GetRayGenRegion(),
                              pipeline->GetMissRegion(),
                              pipeline->GetHitRegion(),
                              pipeline->GetCallableRegion(),
                              extent.width, extent.height, 1);
        }
    });
}
