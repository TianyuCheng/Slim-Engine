#include "composer.h"

void AddComposerPass(RenderGraph& renderGraph,
                     AutoReleasePool& pool,
                     RenderGraph::Resource* colorBuffer,
                     GBuffer* gbuffer,
                     MainScene* scene) {

    // sampler
    static auto sampler = pool.FetchOrCreate(
        "nearest.sampler",
        [&](Device* device) {
            return new Sampler(device, SamplerDesc { });
        });

    // vertex shader
    static auto vShader = pool.FetchOrCreate(
        "fullscreen.vertex.shader",
        [&](Device* device) {
            return new spirv::VertexShader(device, "main", "shaders/fullscreen.vert.spv");
        });

    // fragment shader
    static auto fShader = pool.FetchOrCreate(
        "composer.fragment.shader",
        [&](Device* device) {
            return new spirv::FragmentShader(device, "main", "shaders/compose.frag.spv");
        });

    // pipeline
    static auto pipelineDesc = GraphicsPipelineDesc()
        .SetName("composer")
        .SetVertexShader(vShader)
        .SetFragmentShader(fShader)
        .SetCullMode(VK_CULL_MODE_BACK_BIT)
        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout(PipelineLayoutDesc()
            .AddBinding("Albedo",   SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding("Normal",   SetBinding { 0, 1 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding("Position", SetBinding { 0, 2 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding("Light",    SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT)
        );

    // ------------------------------------------------------------------------------------------------------------------

    // resources
    // NOTE: no additional resources created

    // compile
    auto composerPass = renderGraph.CreateRenderPass("composer");
    composerPass->SetColor(colorBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    composerPass->SetTexture(gbuffer->albedoBuffer);
    composerPass->SetTexture(gbuffer->positionBuffer);
    composerPass->SetTexture(gbuffer->normalBuffer);
    composerPass->SetTexture(gbuffer->depthBuffer);
    composerPass->SetStorage(scene->lightResource, RenderGraph::STORAGE_READ_BIT);

    // execute
    composerPass->Execute([=](const RenderInfo& info) {
        auto pipeline = info.renderFrame->RequestPipeline(
            pipelineDesc
                .SetViewport(info.renderFrame->GetExtent())
                .SetRenderPass(info.renderPass)
        );

        // bind pipeline
        info.commandBuffer->BindPipeline(pipeline);

        // bind descriptor
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetTexture("Albedo", gbuffer->albedoBuffer->GetImage(), sampler);
        descriptor->SetTexture("Normal", gbuffer->normalBuffer->GetImage(), sampler);
        descriptor->SetTexture("Position", gbuffer->positionBuffer->GetImage(), sampler);
        descriptor->SetUniformBuffer("Light", scene->lightBuffer);
        info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

        // draw
        info.commandBuffer->Draw(6, 1, 0, 0);
    });

    return;
}
