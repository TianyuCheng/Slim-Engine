#include "surfel.h"

Sampler* PreparePostSampler(AutoReleasePool& pool) {

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

GraphicsPipelineDesc PrepareComposePass(AutoReleasePool& pool) {

    // vertex shader
    static auto vShader = pool.FetchOrCreate(
        "fullscreen.vertex.shader",
        [&](Device* device) {
            return new spirv::VertexShader(device, "main", "shaders/post/fullscreen.vert.spv");
        });

    // fragment shader
    static auto fShader = pool.FetchOrCreate(
        "composer.fragment.shader",
        [&](Device* device) {
            return new spirv::FragmentShader(device, "main", "shaders/post/compose.frag.spv");
        });

    // pipeline
    static auto pipelineDesc = GraphicsPipelineDesc()
        .SetName("compose")
        .SetVertexShader(vShader)
        .SetFragmentShader(fShader)
        .SetCullMode(VK_CULL_MODE_BACK_BIT)
        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout(PipelineLayoutDesc()
            .AddPushConstant("Control", Range  { 0, sizeof(SurfelDebugControl) }, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding("Albedo",   SetBinding { 0, GBUFFER_ALBEDO_BINDING     }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding("Normal",   SetBinding { 0, GBUFFER_NORMAL_BINDING     }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding("Depth",    SetBinding { 0, GBUFFER_DEPTH_BINDING      }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            #ifdef ENABLE_DIRECT_ILLUMINATION
            .AddBinding("DirectDiffuse", SetBinding { 0, GBUFFER_DIRECT_DIFFUSE_BINDING    }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            #endif
            .AddBinding("GlobalDiffuse", SetBinding { 0, GBUFFER_GLOBAL_DIFFUSE_BINDING    }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding("Debug",    SetBinding { 1, DEBUG_SURFEL_BINDING       }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        );

    return pipelineDesc;
}

void AddComposePass(RenderGraph&           graph,
                    AutoReleasePool&       pool,
                    render::GBuffer*       gbuffer,
                    render::SceneData*     sceneData,
                    render::Surfel*        surfel,
                    render::Debug*         debug,
                    Scene*                 scene,
                    RenderGraph::Resource* colorAttachment) {

    static auto sampler  = PreparePostSampler(pool);
    static auto pipelineDesc = PrepareComposePass(pool);

    // compile
    auto pass = graph.CreateRenderPass("compose");
    pass->SetColor(colorAttachment, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    pass->SetTexture(gbuffer->albedo);
    pass->SetTexture(gbuffer->normal);
    pass->SetTexture(gbuffer->depth);
    #ifdef ENABLE_DIRECT_ILLUMINATION
    pass->SetTexture(gbuffer->directDiffuse);
    #endif
    pass->SetTexture(gbuffer->globalDiffuse);
    pass->SetTexture(debug->surfelDebug);

    pass->Execute([=](const RenderInfo& info) {
        auto pipeline = info.renderFrame->RequestPipeline(
            pipelineDesc
                .SetViewport(info.renderFrame->GetExtent())
                .SetRenderPass(info.renderPass)
        );

        // bind pipeline
        info.commandBuffer->BindPipeline(pipeline);

        // bind descriptor
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetTexture("Albedo", gbuffer->albedo->GetImage(), sampler);
        descriptor->SetTexture("Normal", gbuffer->normal->GetImage(), sampler);
        descriptor->SetTexture("Depth",  gbuffer->depth->GetImage(), sampler);
        descriptor->SetTexture("GlobalDiffuse", gbuffer->globalDiffuse->GetImage(), sampler);
        #ifdef ENABLE_DIRECT_ILLUMINATION
        descriptor->SetTexture("DirectDiffuse", gbuffer->directDiffuse->GetImage(), sampler);
        #endif
        descriptor->SetTexture("Debug", debug->surfelDebug->GetImage(), sampler);
        info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

        // push constant
        info.commandBuffer->PushConstants(pipeline->Layout(), "Control", &scene->surfelDebugControl);

        // draw
        info.commandBuffer->Draw(6, 1, 0, 0);
    });
}
