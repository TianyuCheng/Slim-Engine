#include "direct.h"

Sampler* PrepareDirectSampler(AutoReleasePool& pool) {

    static auto sampler = pool.FetchOrCreate(
        "nearest.sampler",
        [](Device* device) {
        return new Sampler(device,
                SamplerDesc {}
                .MinFilter(VK_FILTER_NEAREST)
                .MagFilter(VK_FILTER_NEAREST));
        });
    return sampler;
}

Pipeline* PrepareDirectLightingPass(AutoReleasePool& pool) {
    // ray gen shader
    static auto rayGenShader = pool.FetchOrCreate(
        "direct.gen",
        [](Device* device) {
            return new spirv::RayGenShader(device, "shaders/raytrace/direct.rgen.spv");
        });
    // scene closest hit shader
    static auto sceneClosestHitShader = pool.FetchOrCreate(
        "scene.closest.hit",
        [](Device* device) {
            return new spirv::ClosestHitShader(device, "shaders/raytrace/scene.rchit.spv");
        });
    // scene miss shader
    static auto sceneMissShader = pool.FetchOrCreate(
        "scene.miss",
        [](Device* device) {
            return new spirv::MissShader(device, "shaders/raytrace/scene.rmiss.spv");
        });
    // surfel ray intersection shader
    static auto shadowMissShader = pool.FetchOrCreate(
        "shadow.miss",
        [](Device* device) {
            return new spirv::MissShader(device, "shaders/raytrace/shadow.rmiss.spv");
        });
    VkDescriptorBindingFlags bindFlags = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
                                       | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    // ray tracing pipeline
    static auto pipeline = pool.FetchOrCreate(
        "direct.raytrace.pipline",
        [&](Device* device) {
            return new Pipeline(
                device,
                RayTracingPipelineDesc()
                    .SetName("direct-lighting")
                    .SetRayGenShader(rayGenShader)
                    .SetMissShader(sceneMissShader)
                    .SetMissShader(shadowMissShader)
                    .SetClosestHitShader(sceneClosestHitShader)
                    .SetMaxRayRecursionDepth(1)
                    .SetPipelineLayout(PipelineLayoutDesc()
                        .AddPushConstant("Info",         Range      { 0, sizeof(uint32_t)       },                                                             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Accel",             SetBinding { 0, SCENE_ACCEL_BINDING    },              VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Frame",             SetBinding { 0, SCENE_FRAME_BINDING    },              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Camera",            SetBinding { 0, SCENE_CAMERA_BINDING   },              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Lights",            SetBinding { 0, SCENE_LIGHT_BINDING    },              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Instances",         SetBinding { 0, SCENE_INSTANCE_BINDING },              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
                        .AddBinding("Materials",         SetBinding { 0, SCENE_MATERIAL_BINDING },              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
                        .AddBinding("Albedo",            SetBinding { 1, GBUFFER_ALBEDO_BINDING },              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("MetallicRoughness", SetBinding { 1, GBUFFER_METALLIC_ROUGHNESS_BINDING },  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Normal",            SetBinding { 1, GBUFFER_NORMAL_BINDING },              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Depth",             SetBinding { 1, GBUFFER_DEPTH_BINDING  },              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        #ifdef ENABLE_GBUFFER_WORLD_POSITION
                        .AddBinding("Position",          SetBinding { 1, GBUFFER_POSITION_BINDING},             VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        #endif
                        .AddBinding("Diffuse",           SetBinding { 1, GBUFFER_DIRECT_DIFFUSE_BINDING},       VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Specular",          SetBinding { 1, GBUFFER_SPECULAR_BINDING},             VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("GDiffuse",          SetBinding { 1, GBUFFER_GLOBAL_DIFFUSE_BINDING},       VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBindingArray("Images",       SetBinding { 2, SCENE_IMAGES_BINDING   }, 1000,        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,              VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, bindFlags)
                        .AddBindingArray("Samplers",     SetBinding { 3, SCENE_SAMPLERS_BINDING }, 1000,        VK_DESCRIPTOR_TYPE_SAMPLER,                    VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, bindFlags)
                        // experiment
                        .AddBinding("Surfel",            SetBinding { 4, SURFEL_BINDING         },              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("SurfelData",        SetBinding { 4, SURFEL_DATA_BINDING    },              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("SurfelLive",        SetBinding { 4, SURFEL_LIVE_BINDING    },              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("SurfelStat",        SetBinding { 4, SURFEL_STAT_BINDING    },              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("SurfelGrid",        SetBinding { 4, SURFEL_GRID_BINDING    },              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("SurfelCell",        SetBinding { 4, SURFEL_CELL_BINDING    },              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("SurfelDepth",       SetBinding { 4, SURFEL_DEPTH_BINDING   },              VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                    )
                );
        }
    );
    return pipeline;
}

void AddDirectLightingPass(RenderGraph&           graph,
                           AutoReleasePool&       pool,
                           render::GBuffer*       gbuffer,
                           render::SceneData*     sceneData,
                           render::Surfel*        surfel,
                           render::Debug*         debug,
                           Scene*                 scene) {

    static auto sampler          = PrepareDirectSampler(pool);
    static auto raytracePipeline = PrepareDirectLightingPass(pool);

    // compile
    auto pass = graph.CreateComputePass("direct");
    pass->SetStorage(sceneData->lights,      RenderGraph::STORAGE_READ_ONLY);
    pass->SetStorage(sceneData->camera,      RenderGraph::STORAGE_READ_ONLY);
    pass->SetStorage(sceneData->frame,       RenderGraph::STORAGE_READ_ONLY);
    pass->SetStorage(gbuffer->directDiffuse, RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetStorage(gbuffer->specular,      RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetStorage(gbuffer->globalDiffuse, RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetTexture(gbuffer->albedo);
    pass->SetTexture(gbuffer->metallicRoughness);
    pass->SetTexture(gbuffer->normal);
    pass->SetTexture(gbuffer->depth);
    #ifdef ENABLE_GBUFFER_WORLD_POSITION
    pass->SetTexture(gbuffer->position);
    #endif

    // execute
    pass->Execute([=](const RenderInfo& info) {
        auto pipeline = raytracePipeline;
        info.commandBuffer->BindPipeline(pipeline);

        // bind descriptor
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetAccelStruct("Accel", scene->builder->GetAccelBuilder()->GetTlas());
        descriptor->SetUniformBuffer("Frame", sceneData->frame->GetBuffer());
        descriptor->SetUniformBuffer("Camera", sceneData->camera->GetBuffer());
        descriptor->SetStorageBuffer("Lights", sceneData->lights->GetBuffer());
        descriptor->SetStorageBuffer("Instances", scene->instanceBuffer);
        descriptor->SetStorageBuffer("Materials", scene->materialBuffer);
        descriptor->SetTexture("Albedo", gbuffer->albedo->GetImage(), sampler);
        descriptor->SetTexture("MetallicRoughness", gbuffer->metallicRoughness->GetImage(), sampler);
        descriptor->SetTexture("Normal", gbuffer->normal->GetImage(), sampler);
        descriptor->SetTexture("Depth", gbuffer->depth->GetImage(), sampler);
        #ifdef ENABLE_GBUFFER_WORLD_POSITION
        descriptor->SetTexture("Position", gbuffer->position->GetImage(), sampler);
        #endif
        descriptor->SetStorageImage("Diffuse", gbuffer->directDiffuse->GetImage());
        descriptor->SetStorageImage("Specular", gbuffer->specular->GetImage());
        descriptor->SetStorageImage("GDiffuse", gbuffer->globalDiffuse->GetImage());
        descriptor->SetSampledImages("Images", scene->images);
        descriptor->SetSamplers("Samplers", scene->samplers);
        // experiment
        descriptor->SetStorageBuffer("Surfel", surfel->surfels->GetBuffer());
        descriptor->SetStorageBuffer("SurfelData", surfel->surfelData->GetBuffer());
        descriptor->SetStorageBuffer("SurfelLive", surfel->surfelLive->GetBuffer());
        descriptor->SetStorageBuffer("SurfelStat", surfel->surfelStat->GetBuffer());
        descriptor->SetStorageBuffer("SurfelGrid", surfel->surfelGrid->GetBuffer());
        descriptor->SetStorageBuffer("SurfelCell", surfel->surfelCell->GetBuffer());
        descriptor->SetStorageImage("SurfelDepth", surfel->surfelDepth->GetImage());
        info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

        uint32_t lightCount = scene->lights.size();
        info.commandBuffer->PushConstants(pipeline->Layout(), "Info", &lightCount);

        // trace rays
        vkCmdTraceRaysKHR(*info.commandBuffer,
                          pipeline->GetRayGenRegion(),
                          pipeline->GetMissRegion(),
                          pipeline->GetHitRegion(),
                          pipeline->GetCallableRegion(),
                          info.renderFrame->GetExtent().width,
                          info.renderFrame->GetExtent().height,
                          1);
    });
}
