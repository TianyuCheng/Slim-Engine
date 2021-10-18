#include "debug.h"

Sampler* PrepareNearestSampler(AutoReleasePool& pool) {

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

Pipeline* PrepareLinearDepthPass(AutoReleasePool& pool) {

    static auto shader = pool.FetchOrCreate(
        "linear.depth.shader",
        [](Device* device) {
            return new spirv::ComputeShader(device, "main", "shaders/debug/depthvis.comp.spv");
        });

    static auto pipeline = pool.FetchOrCreate(
        "linearize.depth.pipeline",
        [&](Device* device) {
            return new Pipeline(
                device,
                ComputePipelineDesc()
                .SetName("linear-depth")
                .SetComputeShader(shader)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("Frame",    SetBinding { 0, SCENE_FRAME_BINDING   }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Camera",   SetBinding { 0, SCENE_CAMERA_BINDING  }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Depth",    SetBinding { 1, GBUFFER_DEPTH_BINDING }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("DepthVis", SetBinding { 2, DEBUG_DEPTH_BINDING   }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_SHADER_STAGE_COMPUTE_BIT)
                )
            );
        });

    return pipeline;
}

Pipeline* PrepareObjectVisPass(AutoReleasePool& pool) {

    static auto shader = pool.FetchOrCreate(
        "object.visualization.shader",
        [](Device* device) {
            return new spirv::ComputeShader(device, "main", "shaders/debug/objectvis.comp.spv");
        });

    static auto pipeline = pool.FetchOrCreate(
        "object.visualization.pipeline",
        [&](Device* device) {
            return new Pipeline(
                device,
                ComputePipelineDesc()
                .SetName("object-visualization")
                .SetComputeShader(shader)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("Frame",     SetBinding { 0, SCENE_FRAME_BINDING    }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Camera",    SetBinding { 0, SCENE_CAMERA_BINDING   }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Object",    SetBinding { 1, GBUFFER_OBJECT_BINDING }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("ObjectVis", SetBinding { 2, DEBUG_OBJECT_BINDING   }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_SHADER_STAGE_COMPUTE_BIT)
                )
            );
        });

    return pipeline;
}

Pipeline* PrepareGridVisPass(AutoReleasePool& pool) {

    static auto shader = pool.FetchOrCreate(
        "grid.visualization.shader",
        [](Device* device) {
            return new spirv::ComputeShader(device, "main", "shaders/debug/gridvis.comp.spv");
        });

    static auto pipeline = pool.FetchOrCreate(
        "grid.visualization.pipeline",
        [&](Device* device) {
            return new Pipeline(
                device,
                ComputePipelineDesc()
                .SetName("surfel-grid-visualization")
                .SetComputeShader(shader)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("Frame",     SetBinding { 0, SCENE_FRAME_BINDING       }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Camera",    SetBinding { 0, SCENE_CAMERA_BINDING      }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Albedo",    SetBinding { 1, GBUFFER_ALBEDO_BINDING    }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Depth",     SetBinding { 1, GBUFFER_DEPTH_BINDING     }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("GridVis",   SetBinding { 2, DEBUG_GRID_BINDING        }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_SHADER_STAGE_COMPUTE_BIT)
                )
            );
        });

    return pipeline;
}

Pipeline* PrepareSurfelAllocVisPass(AutoReleasePool& pool) {

    static auto shader = pool.FetchOrCreate(
        "surfel.allocation.visualization.shader",
        [](Device* device) {
            return new spirv::ComputeShader(device, "main", "shaders/debug/allocvis.comp.spv");
        });

    static auto pipeline = pool.FetchOrCreate(
        "surfel.allocation.visualization.pipeline",
        [&](Device* device) {
            return new Pipeline(
                device,
                ComputePipelineDesc()
                .SetName("surfel-allocation-visualization")
                .SetComputeShader(shader)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("SurfelStat",   SetBinding { 0, SURFEL_STAT_BINDING         }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("SurfelBudget", SetBinding { 1, DEBUG_SURFEL_BUDGET_BINDING }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  VK_SHADER_STAGE_COMPUTE_BIT)
                )
            );
        });

    return pipeline;
}

void AddLinearDepthPass(RenderGraph&       graph,
                        AutoReleasePool&   pool,
                        render::GBuffer*   gbuffer,
                        render::SceneData* sceneData,
                        render::Surfel*    surfel,
                        render::Debug*     debug,
                        Scene*             scene) {

    static auto sampler = PrepareNearestSampler(pool);
    static auto pipeline = PrepareLinearDepthPass(pool);

    auto pass = graph.CreateComputePass("linear-depth");
    pass->SetStorage(sceneData->frame,  RenderGraph::STORAGE_READ_ONLY);
    pass->SetStorage(sceneData->camera, RenderGraph::STORAGE_READ_ONLY);
    pass->SetStorage(debug->depth, RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetTexture(gbuffer->depth);
    pass->Execute([=](const RenderInfo &info) {

        // bind pipeline
        info.commandBuffer->BindPipeline(pipeline);

        // bind camera uniform
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetUniformBuffer("Frame", scene->frameInfoBuffer);
        descriptor->SetUniformBuffer("Camera", scene->cameraBuffer);
        descriptor->SetTexture("Depth", gbuffer->depth->GetImage(), sampler);
        descriptor->SetStorageImage("DepthVis", debug->depth->GetImage());
        info.commandBuffer->BindDescriptor(descriptor, VK_PIPELINE_BIND_POINT_COMPUTE);

        // dispatch compute
        const auto& extent = info.renderFrame->GetExtent();
        uint32_t x = ((extent.width + 15) / 16);
        uint32_t y = ((extent.height + 15) / 16);
        info.commandBuffer->Dispatch(x, y, 1);
    });
}

void AddObjectVisPass(RenderGraph&       graph,
                      AutoReleasePool&   pool,
                      render::GBuffer*   gbuffer,
                      render::SceneData* sceneData,
                      render::Surfel*    surfel,
                      render::Debug*     debug,
                      Scene*             scene) {

    static auto sampler = PrepareNearestSampler(pool);
    static auto pipeline = PrepareObjectVisPass(pool);

    auto pass = graph.CreateComputePass("object-vis");
    pass->SetStorage(sceneData->frame,  RenderGraph::STORAGE_READ_ONLY);
    pass->SetStorage(sceneData->camera, RenderGraph::STORAGE_READ_ONLY);
    pass->SetStorage(debug->object, RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetTexture(gbuffer->object);
    pass->Execute([=](const RenderInfo &info) {

        // bind pipeline
        info.commandBuffer->BindPipeline(pipeline);

        // bind camera uniform
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetUniformBuffer("Frame", scene->frameInfoBuffer);
        descriptor->SetUniformBuffer("Camera", scene->cameraBuffer);
        descriptor->SetTexture("Object", gbuffer->object->GetImage(), sampler);
        descriptor->SetStorageImage("ObjectVis", debug->object->GetImage());
        info.commandBuffer->BindDescriptor(descriptor, VK_PIPELINE_BIND_POINT_COMPUTE);

        // dispatch compute
        const auto& extent = info.renderFrame->GetExtent();
        uint32_t x = ((extent.width + 15) / 16);
        uint32_t y = ((extent.height + 15) / 16);
        info.commandBuffer->Dispatch(x, y, 1);
    });

}

void AddGridVisPass(RenderGraph&       graph,
                    AutoReleasePool&   pool,
                    render::GBuffer*   gbuffer,
                    render::SceneData* sceneData,
                    render::Surfel*    surfel,
                    render::Debug*     debug,
                    Scene*             scene) {

    static auto sampler = PrepareNearestSampler(pool);
    static auto pipeline = PrepareGridVisPass(pool);

    auto pass = graph.CreateComputePass("grid-vis");
    pass->SetStorage(sceneData->frame,  RenderGraph::STORAGE_READ_ONLY);
    pass->SetStorage(sceneData->camera, RenderGraph::STORAGE_READ_ONLY);
    pass->SetStorage(debug->surfelGrid, RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetTexture(gbuffer->albedo);
    pass->SetTexture(gbuffer->depth);
    pass->Execute([=](const RenderInfo &info) {

        // bind pipeline
        info.commandBuffer->BindPipeline(pipeline);

        // bind camera uniform
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetUniformBuffer("Frame", scene->frameInfoBuffer);
        descriptor->SetUniformBuffer("Camera", scene->cameraBuffer);
        descriptor->SetTexture("Albedo", gbuffer->albedo->GetImage(), sampler);
        descriptor->SetTexture("Depth", gbuffer->depth->GetImage(), sampler);
        descriptor->SetStorageImage("GridVis", debug->surfelGrid->GetImage());
        info.commandBuffer->BindDescriptor(descriptor, VK_PIPELINE_BIND_POINT_COMPUTE);

        // dispatch compute
        const auto& extent = info.renderFrame->GetExtent();
        uint32_t x = ((extent.width + 15) / 16);
        uint32_t y = ((extent.height + 15) / 16);
        info.commandBuffer->Dispatch(x, y, 1);
    });

}

void AddSurfelAllocVisPass(RenderGraph&       graph,
                           AutoReleasePool&   pool,
                           render::GBuffer*   gbuffer,
                           render::SceneData* sceneData,
                           render::Surfel*    surfel,
                           render::Debug*     debug,
                           Scene*             scene) {

    static auto pipeline = PrepareSurfelAllocVisPass(pool);

    auto pass = graph.CreateComputePass("surfel-alloc-vis");
    pass->SetStorage(surfel->surfelStat, RenderGraph::STORAGE_READ_ONLY);
    pass->SetStorage(debug->surfelBudget, RenderGraph::STORAGE_WRITE_ONLY);
    pass->Execute([=](const RenderInfo &info) {

        // bind pipeline
        info.commandBuffer->BindPipeline(pipeline);

        // bind camera uniform
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetStorageBuffer("SurfelStat", surfel->surfelStat->GetBuffer());
        descriptor->SetStorageImage("SurfelBudget", debug->surfelBudget->GetImage());
        info.commandBuffer->BindDescriptor(descriptor, VK_PIPELINE_BIND_POINT_COMPUTE);

        // dispatch compute
        info.commandBuffer->Dispatch(20, 1, 1);
    });
}
