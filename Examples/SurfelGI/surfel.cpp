#include "surfel.h"
#include "shaders/common.h"

Sampler* PrepareSurfelSampler(AutoReleasePool& pool) {

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

Pipeline* PrepareSurfelPreparePass(AutoReleasePool& pool) {

    static auto shader = pool.FetchOrCreate(
        "surfel.prepare.shader",
        [](Device* device) {
            return new spirv::ComputeShader(device, "shaders/surfel/prepare.comp.spv");
        });

    static auto pipeline = pool.FetchOrCreate(
        "surfel.prepare.pipeline",
        [&](Device* device) {
            return new Pipeline(
                device,
                ComputePipelineDesc()
                .SetName("surfel-prepare")
                .SetComputeShader(shader)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("SurfelStat", SetBinding { 0, SURFEL_STAT_BINDING }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                )
            );
        });

    return pipeline;
}

Pipeline* PrepareSurfelResetPass(AutoReleasePool& pool) {

    static auto shader = pool.FetchOrCreate(
        "surfel.reset.shader",
        [](Device* device) {
            return new spirv::ComputeShader(device, "shaders/surfel/reset.comp.spv");
        });

    static auto pipeline = pool.FetchOrCreate(
        "surfel.reset.pipeline",
        [&](Device* device) {
            return new Pipeline(
                device,
                ComputePipelineDesc()
                .SetName("surfel-reset")
                .SetComputeShader(shader)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("SurfelGrid", SetBinding { 0, SURFEL_GRID_BINDING }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                )
            );
        });

    return pipeline;
}

Pipeline* PrepareSurfelOffsetPass(AutoReleasePool& pool) {

    static auto shader = pool.FetchOrCreate(
        "surfel.offset.shader",
        [](Device* device) {
            return new spirv::ComputeShader(device, "shaders/surfel/offset.comp.spv");
        });

    static auto pipeline = pool.FetchOrCreate(
        "surfel.offset.pipeline",
        [&](Device* device) {
            return new Pipeline(
                device,
                ComputePipelineDesc()
                .SetName("surfel-offset")
                .SetComputeShader(shader)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("SurfelStat", SetBinding { 0, SURFEL_STAT_BINDING }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("SurfelGrid", SetBinding { 0, SURFEL_GRID_BINDING }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                )
            );
        });

    return pipeline;
}

Pipeline* PrepareSurfelUpdatePass(AutoReleasePool& pool) {

    static auto shader = pool.FetchOrCreate(
        "surfel.update.shader",
        [](Device* device) {
            return new spirv::ComputeShader(device, "shaders/surfel/update.comp.spv");
        });

    static auto pipeline = pool.FetchOrCreate(
        "surfel.update.pipeline",
        [&](Device* device) {
            return new Pipeline(
                device,
                ComputePipelineDesc()
                .SetName("surfel-update")
                .SetComputeShader(shader)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("Camera",     SetBinding { 0, SCENE_CAMERA_BINDING  }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Surfel",     SetBinding { 1, SURFEL_BINDING        }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("SurfelLive", SetBinding { 1, SURFEL_LIVE_BINDING   }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("SurfelData", SetBinding { 1, SURFEL_DATA_BINDING   }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("SurfelGrid", SetBinding { 1, SURFEL_GRID_BINDING   }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("SurfelStat", SetBinding { 1, SURFEL_STAT_BINDING   }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                )
            );
        });

    return pipeline;
}

Pipeline* PrepareSurfelBinningPass(AutoReleasePool& pool) {

    static auto shader = pool.FetchOrCreate(
        "surfel.binning.shader",
        [](Device* device) {
            return new spirv::ComputeShader(device, "shaders/surfel/binning.comp.spv");
        });

    static auto pipeline = pool.FetchOrCreate(
        "surfel.binning.pipeline",
        [&](Device* device) {
            return new Pipeline(
                device,
                ComputePipelineDesc()
                .SetName("surfel-binning")
                .SetComputeShader(shader)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("Camera",     SetBinding { 0, SCENE_CAMERA_BINDING  }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Surfel",     SetBinding { 1, SURFEL_BINDING        }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("SurfelLive", SetBinding { 1, SURFEL_LIVE_BINDING   }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("SurfelStat", SetBinding { 1, SURFEL_STAT_BINDING   }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("SurfelGrid", SetBinding { 1, SURFEL_GRID_BINDING   }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("SurfelCell", SetBinding { 1, SURFEL_CELL_BINDING   }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                )
            );
        });

    return pipeline;
}

Pipeline* PrepareSurfelCoveragePass(AutoReleasePool& pool) {

    static auto shader = pool.FetchOrCreate(
        "surfel.coverage.shader",
        [](Device* device) {
            return new spirv::ComputeShader(device, "shaders/surfel/coverage.comp.spv");
        });

    static auto pipeline = pool.FetchOrCreate(
        "surfel.coverage.pipeline",
        [&](Device* device) {
            return new Pipeline(
                device,
                ComputePipelineDesc()
                .SetName("surfel-coverage")
                .SetComputeShader(shader)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddPushConstant("Control", Range  { 0, sizeof(DebugControl)             },                                            VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Frame",      SetBinding { 0, SCENE_FRAME_BINDING            }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Camera",     SetBinding { 0, SCENE_CAMERA_BINDING           }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Surfel",     SetBinding { 1, SURFEL_BINDING                 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("SurfelData", SetBinding { 1, SURFEL_DATA_BINDING            }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("SurfelLive", SetBinding { 1, SURFEL_LIVE_BINDING            }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("SurfelStat", SetBinding { 1, SURFEL_STAT_BINDING            }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("SurfelGrid", SetBinding { 1, SURFEL_GRID_BINDING            }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("SurfelCell", SetBinding { 1, SURFEL_CELL_BINDING            }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("SurfelDepth",SetBinding { 1, SURFEL_DEPTH_BINDING           }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Coverage",   SetBinding { 1, SURFEL_COVERAGE_BINDING        }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Diffuse",    SetBinding { 2, GBUFFER_GLOBAL_DIFFUSE_BINDING }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Albedo",     SetBinding { 2, GBUFFER_ALBEDO_BINDING         }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Depth",      SetBinding { 2, GBUFFER_DEPTH_BINDING          }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Normal",     SetBinding { 2, GBUFFER_NORMAL_BINDING         }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
                    #ifdef ENABLE_GBUFFER_WORLD_POSITION
                    .AddBinding("Position",   SetBinding { 2, GBUFFER_POSITION_BINDING       }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
                    #endif
                    .AddBinding("Object",     SetBinding { 2, GBUFFER_OBJECT_BINDING         }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Debug",      SetBinding { 3, DEBUG_SURFEL_BINDING           }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Variance",   SetBinding { 3, DEBUG_SURFEL_VAR_BINDING       }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_SHADER_STAGE_COMPUTE_BIT)
                )
            );
        });

    return pipeline;
}

Pipeline* PrepareRayTracePass(AutoReleasePool& pool) {
    // ray gen shader
    static auto rayGenShader = pool.FetchOrCreate(
        "ray.gen",
        [](Device* device) {
            return new spirv::RayGenShader(device, "shaders/raytrace/ray.rgen.spv");
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
        "surfel.raytrace.pipline",
        [&](Device* device) {
            return new Pipeline(
                device,
                RayTracingPipelineDesc()
                    .SetName("ray-tracing")
                    .SetRayGenShader(rayGenShader)
                    .SetMissShader(sceneMissShader)
                    .SetMissShader(shadowMissShader)
                    .SetClosestHitShader(sceneClosestHitShader)
                    .SetMaxRayRecursionDepth(1)
                    .SetPipelineLayout(PipelineLayoutDesc()
                        .AddPushConstant("Info",       Range      { 0, sizeof(uint32_t)       },                                                      VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Accel",           SetBinding { 0, SCENE_ACCEL_BINDING    },       VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Frame",           SetBinding { 0, SCENE_FRAME_BINDING    },       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Camera",          SetBinding { 0, SCENE_CAMERA_BINDING   },       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Sky",             SetBinding { 0, SCENE_SKY_BINDING      },       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Lights",          SetBinding { 0, SCENE_LIGHT_BINDING    },       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("Instances",       SetBinding { 0, SCENE_INSTANCE_BINDING },       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
                        .AddBinding("Materials",       SetBinding { 0, SCENE_MATERIAL_BINDING },       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
                        .AddBinding("Surfel",          SetBinding { 1, SURFEL_BINDING         },       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("SurfelLive",      SetBinding { 1, SURFEL_LIVE_BINDING    },       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("SurfelData",      SetBinding { 1, SURFEL_DATA_BINDING    },       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("SurfelGrid",      SetBinding { 1, SURFEL_GRID_BINDING    },       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("SurfelCell",      SetBinding { 1, SURFEL_CELL_BINDING    },       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("SurfelStat",      SetBinding { 1, SURFEL_STAT_BINDING    },       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("SurfelDepth",     SetBinding { 1, SURFEL_DEPTH_BINDING   },       VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBinding("SurfelRayGuide",  SetBinding { 1, SURFEL_RAYGUIDE_BINDING},       VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                        .AddBindingArray("Images",     SetBinding { 2, SCENE_IMAGES_BINDING   }, 1000, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,              VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, bindFlags)
                        .AddBindingArray("Samplers",   SetBinding { 3, SCENE_SAMPLERS_BINDING }, 1000, VK_DESCRIPTOR_TYPE_SAMPLER,                    VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, bindFlags)
                        .AddBinding("SampleRays",      SetBinding { 4, DEBUG_SURFEL_RAYDIR_BINDING},   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                    )
                );
        }
    );
    return pipeline;
}

void AddSurfelPass(RenderGraph&       graph,
                   AutoReleasePool&   pool,
                   render::GBuffer*   gbuffer,
                   render::SceneData* sceneData,
                   render::Surfel*    surfel,
                   render::Debug*     debug,
                   Scene*             scene) {

    static auto sampler          = PrepareSurfelSampler(pool);
    static auto preparePipeline  = PrepareSurfelPreparePass(pool);
    static auto updatePipeline   = PrepareSurfelUpdatePass(pool);
    static auto resetPipeline    = PrepareSurfelResetPass(pool);
    static auto offsetPipeline   = PrepareSurfelOffsetPass(pool);
    static auto binningPipeline  = PrepareSurfelBinningPass(pool);
    static auto coveragePipeline = PrepareSurfelCoveragePass(pool);
    static auto raytracePipeline = PrepareRayTracePass(pool);

    uint32_t groupCountX = (SURFEL_GRID_COUNT + SURFEL_UPDATE_GROUP_SIZE - 1) / SURFEL_UPDATE_GROUP_SIZE;
    uint32_t groupCountY = 1;
    uint32_t groupCountZ = 1;

    // compile
    auto pass = graph.CreateComputePass("surfel");
    pass->SetStorage(sceneData->sky,         RenderGraph::STORAGE_READ_ONLY);
    pass->SetStorage(sceneData->lights,      RenderGraph::STORAGE_READ_ONLY);
    pass->SetStorage(sceneData->camera,      RenderGraph::STORAGE_READ_ONLY);
    pass->SetStorage(sceneData->frame,       RenderGraph::STORAGE_READ_ONLY);
    pass->SetStorage(surfel->surfels,        RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetStorage(surfel->surfelGrid,     RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetStorage(surfel->surfelStat,     RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetStorage(surfel->surfelLive,     RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetStorage(surfel->surfelStat,     RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetStorage(surfel->surfelData,     RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetStorage(surfel->surfelCoverage, RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetStorage(surfel->surfelDepth,    RenderGraph::STORAGE_READ_WRITE);
    pass->SetStorage(surfel->surfelRayGuide, RenderGraph::STORAGE_READ_WRITE);
    pass->SetStorage(debug->surfelDebug,     RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetStorage(debug->surfelVariance,  RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetStorage(debug->sampleRays,      RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetStorage(gbuffer->globalDiffuse, RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetTexture(gbuffer->albedo);
    pass->SetTexture(gbuffer->normal);
    pass->SetTexture(gbuffer->depth);
    pass->SetTexture(gbuffer->object);
    #ifdef ENABLE_GBUFFER_WORLD_POSITION
    pass->SetTexture(gbuffer->position);
    #endif

    pass->Execute([=](const RenderInfo& info) {
        debug->surfelDebug->GetImage()->SetName("SurfelDebug");
        debug->surfelVariance->GetImage()->SetName("SurfelVariance");
        surfel->surfelDepth->GetImage()->SetName("SurfelDepth");
        surfel->surfelRayGuide->GetImage()->SetName("SurfelRayGuide");

        // Step: positional update (for every surfel)
        // We don't plan to support this. Skip for now.

        // Step: recycle (for every surfel)
        // We don't plan to support this. Skip for now.

        // Step: reset and prepare for indirection update
        info.commandBuffer->BeginRegion("surfel-prepare");
        {
            auto pipeline = preparePipeline;
            info.commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetStorageBuffer("SurfelStat", surfel->surfelStat->GetBuffer());
            info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

            // issue compute call
            info.commandBuffer->Dispatch(1, 1, 1);

            // barriers
            info.commandBuffer->PrepareForBuffer(surfel->surfelStat->GetBuffer(),
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
        info.commandBuffer->EndRegion();

        // Step: clear grid information
        info.commandBuffer->BeginRegion("surfel-reset");
        {
            auto pipeline = resetPipeline;
            info.commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetStorageBuffer("SurfelGrid", surfel->surfelGrid->GetBuffer());
            info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

            // issue compute call
            info.commandBuffer->Dispatch(groupCountX, groupCountY, groupCountZ);

            // barriers
            info.commandBuffer->PrepareForBuffer(surfel->surfelGrid->GetBuffer(),
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
        info.commandBuffer->EndRegion();

        // Step: update surfel information (should be done before)
        // and update grid count
        info.commandBuffer->BeginRegion("surfel-update");
        {
            auto pipeline = updatePipeline;
            info.commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetUniformBuffer("Camera",     sceneData->camera->GetBuffer());
            descriptor->SetStorageBuffer("Surfel",     surfel->surfels->GetBuffer());
            descriptor->SetStorageBuffer("SurfelLive", surfel->surfelLive->GetBuffer());
            descriptor->SetStorageBuffer("SurfelData", surfel->surfelData->GetBuffer());
            descriptor->SetStorageBuffer("SurfelGrid", surfel->surfelGrid->GetBuffer());
            descriptor->SetStorageBuffer("SurfelStat", surfel->surfelStat->GetBuffer());
            info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

            // issue compute call
            vkCmdDispatchIndirect(*info.commandBuffer, *surfel->surfelStat->GetBuffer(), offsetof(SurfelStat, x));

            // barriers
            info.commandBuffer->PrepareForBuffer(surfel->surfelGrid->GetBuffer(),
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
        info.commandBuffer->EndRegion();

        // Step: grid alloc
        info.commandBuffer->BeginRegion("surfel-offset");
        {
            auto pipeline = offsetPipeline;
            info.commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetStorageBuffer("SurfelStat", surfel->surfelStat->GetBuffer());
            descriptor->SetStorageBuffer("SurfelGrid", surfel->surfelGrid->GetBuffer());
            info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

            // issue compute call
            info.commandBuffer->Dispatch(groupCountX, groupCountY, groupCountZ);

            // barriers
            info.commandBuffer->PrepareForBuffer(surfel->surfelStat->GetBuffer(),
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            info.commandBuffer->PrepareForBuffer(surfel->surfelGrid->GetBuffer(),
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
        info.commandBuffer->EndRegion();

        // Step: grid binning
        info.commandBuffer->BeginRegion("surfel-binning");
        {
            auto pipeline = binningPipeline;
            info.commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetUniformBuffer("Camera", sceneData->camera->GetBuffer());
            descriptor->SetStorageBuffer("Surfel", surfel->surfels->GetBuffer());
            descriptor->SetStorageBuffer("SurfelLive", surfel->surfelLive->GetBuffer());
            descriptor->SetStorageBuffer("SurfelStat", surfel->surfelStat->GetBuffer());
            descriptor->SetStorageBuffer("SurfelGrid", surfel->surfelGrid->GetBuffer());
            descriptor->SetStorageBuffer("SurfelCell", surfel->surfelCell->GetBuffer());
            info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

            // issue compute call
            vkCmdDispatchIndirect(*info.commandBuffer, *surfel->surfelStat->GetBuffer(), offsetof(SurfelStat, x));

            // barriers
            info.commandBuffer->PrepareForBuffer(surfel->surfelCell->GetBuffer(),
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
        info.commandBuffer->EndRegion();

        // Step: determine ray count, update lighting
        info.commandBuffer->BeginRegion("raytrace");
        {
            auto pipeline = raytracePipeline;
            info.commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetAccelStruct("Accel", scene->builder->GetAccelBuilder()->GetTlas());
            descriptor->SetUniformBuffer("Frame", sceneData->frame->GetBuffer());
            descriptor->SetUniformBuffer("Camera", sceneData->camera->GetBuffer());
            descriptor->SetStorageBuffer("Sky", sceneData->sky->GetBuffer());
            descriptor->SetStorageBuffer("Lights", sceneData->lights->GetBuffer());
            descriptor->SetStorageBuffer("Instances", scene->instanceBuffer);
            descriptor->SetStorageBuffer("Materials", scene->materialBuffer);
            descriptor->SetStorageBuffer("Surfel", surfel->surfels->GetBuffer());
            descriptor->SetStorageBuffer("SurfelLive", surfel->surfelLive->GetBuffer());
            descriptor->SetStorageBuffer("SurfelData", surfel->surfelData->GetBuffer());
            descriptor->SetStorageBuffer("SurfelStat", surfel->surfelStat->GetBuffer());
            descriptor->SetStorageBuffer("SurfelGrid", surfel->surfelGrid->GetBuffer());
            descriptor->SetStorageBuffer("SurfelCell", surfel->surfelCell->GetBuffer());
            descriptor->SetStorageImage("SurfelDepth", surfel->surfelDepth->GetImage());
            descriptor->SetStorageImage("SurfelRayGuide", surfel->surfelRayGuide->GetImage());
            descriptor->SetSampledImages("Images", scene->images);
            descriptor->SetSamplers("Samplers", scene->samplers);
            descriptor->SetStorageBuffer("SampleRays", debug->sampleRays->GetBuffer());
            info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

            uint32_t lightCount = scene->lights.size();
            info.commandBuffer->PushConstants(pipeline->Layout(), "Info", &lightCount);

            // trace rays
            vkCmdTraceRaysKHR(*info.commandBuffer,
                              pipeline->GetRayGenRegion(),
                              pipeline->GetMissRegion(),
                              pipeline->GetHitRegion(),
                              pipeline->GetCallableRegion(),
                              SURFEL_CAPACITY_SQRT, SURFEL_CAPACITY_SQRT, 1);

            // barriers
            info.commandBuffer->PrepareForBuffer(surfel->surfelData->GetBuffer(),
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
        info.commandBuffer->EndRegion();

        // ----------------------------------------------------------------------------------------------------

        // Step: gap detect, gap fill
        info.commandBuffer->BeginRegion("surfel-coverage");
        {
            auto pipeline = coveragePipeline;
            info.commandBuffer->BindPipeline(pipeline);

            // bind descriptor
            auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
            descriptor->SetUniformBuffer("Frame", sceneData->frame->GetBuffer());
            descriptor->SetUniformBuffer("Camera", sceneData->camera->GetBuffer());
            descriptor->SetStorageBuffer("Surfel", surfel->surfels->GetBuffer());
            descriptor->SetStorageBuffer("SurfelData", surfel->surfelData->GetBuffer());
            descriptor->SetStorageBuffer("SurfelLive", surfel->surfelLive->GetBuffer());
            descriptor->SetStorageBuffer("SurfelStat", surfel->surfelStat->GetBuffer());
            descriptor->SetStorageBuffer("SurfelGrid", surfel->surfelGrid->GetBuffer());
            descriptor->SetStorageBuffer("SurfelCell", surfel->surfelCell->GetBuffer());
            descriptor->SetStorageImage("Coverage", surfel->surfelCoverage->GetImage());
            descriptor->SetStorageImage("SurfelDepth", surfel->surfelDepth->GetImage());
            descriptor->SetStorageImage("Debug", debug->surfelDebug->GetImage());
            descriptor->SetStorageImage("Variance", debug->surfelVariance->GetImage());
            descriptor->SetStorageImage("Diffuse", gbuffer->globalDiffuse->GetImage());
            descriptor->SetTexture("Albedo", gbuffer->albedo->GetImage(), sampler);
            descriptor->SetTexture("Depth",  gbuffer->depth->GetImage(), sampler);
            descriptor->SetTexture("Normal", gbuffer->normal->GetImage(), sampler);
            descriptor->SetTexture("Object", gbuffer->object->GetImage(), sampler);
            #ifdef ENABLE_GBUFFER_WORLD_POSITION
            descriptor->SetTexture("Position", gbuffer->position->GetImage(), sampler);
            #endif
            info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());
            info.commandBuffer->PushConstants(pipeline->Layout(), "Control", &scene->debugControl);

            // issue compute call
            #ifdef ENABLE_HALFRES_LIGHT_APPLY
            uint32_t groupX = ((info.renderFrame->GetExtent().width  >> 1) + SURFEL_TILE_X - 1) / SURFEL_TILE_X;
            uint32_t groupY = ((info.renderFrame->GetExtent().height >> 1) + SURFEL_TILE_Y - 1) / SURFEL_TILE_Y;
            #else
            uint32_t groupX = (info.renderFrame->GetExtent().width  + SURFEL_TILE_X - 1) / SURFEL_TILE_X;
            uint32_t groupY = (info.renderFrame->GetExtent().height + SURFEL_TILE_Y - 1) / SURFEL_TILE_Y;
            #endif
            info.commandBuffer->Dispatch(groupX, groupY, 1);

            // barriers
            info.commandBuffer->PrepareForBuffer(surfel->surfelStat->GetBuffer(),
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            info.commandBuffer->PrepareForBuffer(surfel->surfelLive->GetBuffer(),
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            info.commandBuffer->PrepareForBuffer(surfel->surfelData->GetBuffer(),
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
        info.commandBuffer->EndRegion();

    });
}
