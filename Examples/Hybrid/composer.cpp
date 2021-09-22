#include "composer.h"

void AddComposerPass(RenderGraph& renderGraph,
                     ResourceBundle& bundle,
                     RenderGraph::Resource* colorBuffer,
                     GBuffer* gbuffer,
                     DirectionalLight* light) {

    Device* device = renderGraph.GetRenderFrame()->GetDevice();

    // sampler
    static Sampler* sampler = nullptr;
    if (sampler == nullptr) {
        sampler = new Sampler(device, SamplerDesc { });
        bundle.AutoRelease(sampler);
    }

    // vertex shader
    static Shader* vShader = nullptr;
    if (vShader == nullptr) {
        vShader = new spirv::VertexShader(device, "main", "shaders/compose.vert.spv");
        bundle.AutoRelease(vShader);
    }

    // fragment shader
    static Shader* fShader = nullptr;
    if (fShader == nullptr) {
        fShader = new spirv::FragmentShader(device, "main", "shaders/compose.frag.spv");
        bundle.AutoRelease(fShader);
    }

    // pipeline
    static auto pipelineDesc = GraphicsPipelineDesc()
        .SetName("composer")
        .SetVertexShader(vShader)
        .SetFragmentShader(fShader)
        .SetCullMode(VK_CULL_MODE_BACK_BIT)
        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout(PipelineLayoutDesc()
            .AddPushConstant("DirectionalLight", Range { 0, sizeof(DirectionalLight) }, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding("Albedo",   SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding("Normal",   SetBinding { 0, 1 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding("Position", SetBinding { 0, 2 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        );

    // ------------------------------------------------------------------------------------------------------------------

    // resources
    // NOTE: no additional resources created

    // compile
    auto composerPass = renderGraph.CreateRenderPass("composer");
    composerPass->SetColor(colorBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    composerPass->SetTexture(gbuffer->albedoBuffer);
    composerPass->SetTexture(gbuffer->normalBuffer);
    composerPass->SetTexture(gbuffer->positionBuffer);
    composerPass->SetTexture(gbuffer->depthBuffer);

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
        info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());
        info.commandBuffer->PushConstants(pipeline->Layout(), "DirectionalLight", light);

        // draw
        info.commandBuffer->Draw(6, 1, 0, 0);
    });

    return;
}
