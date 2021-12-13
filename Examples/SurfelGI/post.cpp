#include "post.h"

Sampler* PrepareLinearSampler(AutoReleasePool& pool) {

    static auto sampler = pool.FetchOrCreate(
        "linear.sampler",
        [](Device* device) {
        return new Sampler(device,
                SamplerDesc {}
                .MinFilter(VK_FILTER_LINEAR)
                .MagFilter(VK_FILTER_LINEAR));
        });
    return sampler;
}

GraphicsPipelineDesc PrepareBilateralPass(AutoReleasePool& pool) {
    // vertex shader
    auto vShader = pool.FetchOrCreate(
        "bilateral.vertex.shader",
        [](Device* device) {
            return new spirv::VertexShader(device, "main", "shaders/post/bilateral.vert.spv");
        });

    // fragment shader
    auto fShader = pool.FetchOrCreate(
        "bilateral.fragment.shader",
        [](Device* device) {
            return new spirv::FragmentShader(device, "main", "shaders/post/bilateral.frag.spv");
        });

    // pipeline
    auto pipelineDesc = GraphicsPipelineDesc()
        .SetName("bilaterial")
        .SetVertexShader(vShader)
        .SetFragmentShader(fShader)
        .SetCullMode(VK_CULL_MODE_BACK_BIT)
        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetDepthTest(VK_COMPARE_OP_LESS)
        .SetDepthWrite(VK_FALSE)
        .SetDefaultBlendState(0)
        .SetPipelineLayout(PipelineLayoutDesc()
            .AddPushConstant("Data", Range { 0, sizeof(BilateralInfo) }, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
        );

    return pipelineDesc;
}

void AddDiffuseSmooth(RenderGraph&       graph,
                      AutoReleasePool&   pool,
                      render::GBuffer*   gbuffer,
                      render::SceneData* sceneData,
                      render::Surfel*    surfel,
                      render::Debug*     debug,
                      Scene*             scene) {

    static auto pipelineDesc = PrepareBilateralPass(pool);

    auto pass = graph.CreateRenderPass("bilateral");
    pass->SetColor(gbuffer->globalDiffuse);
    pass->Execute([=](const RenderInfo &info) {

        // bind pipeline
        auto pipeline = info.renderFrame->RequestPipeline(
            pipelineDesc
                .SetRenderPass(info.renderPass)
                .SetViewport(info.renderFrame->GetExtent()));

        info.commandBuffer->BindPipeline(pipeline);

        // bind camera uniform
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetUniformBuffer("Data", BufferAlloc {
            sceneData->camera->GetBuffer(),
            0, sizeof(BilateralInfo)
        });
        info.commandBuffer->BindDescriptor(descriptor, VK_PIPELINE_BIND_POINT_GRAPHICS);

        info.commandBuffer->Draw(6, 1, 0, 0);
    });

}
