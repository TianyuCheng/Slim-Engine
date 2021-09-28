#include "composer.h"

void AddObjectVisPass(RenderGraph& renderGraph,
                      ResourceBundle& bundle,
                      RenderGraph::Resource* targetBuffer,
                      RenderGraph::Resource* objectBuffer) {

    Device* device = renderGraph.GetRenderFrame()->GetDevice();

    // sampler
    static Sampler* sampler = bundle.AutoRelease(new Sampler(device,
        SamplerDesc { }
            .MagFilter(VK_FILTER_NEAREST)
            .MinFilter(VK_FILTER_NEAREST)));

    // vertex shader
    static Shader* vShader = bundle.AutoRelease(new spirv::VertexShader(device, "main", "shaders/fullscreen.vert.spv"));

    // fragment shader
    static Shader* fShader = bundle.AutoRelease(new spirv::FragmentShader(device, "main", "shaders/objectvis.frag.spv"));

    // pipeline
    static auto pipelineDesc = GraphicsPipelineDesc()
        .SetName("objectvis")
        .SetVertexShader(vShader)
        .SetFragmentShader(fShader)
        .SetCullMode(VK_CULL_MODE_BACK_BIT)
        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout(PipelineLayoutDesc()
            .AddBinding("Object",   SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        );

    // ------------------------------------------------------------------------------------------------------------------

    // resources
    // NOTE: no additional resources created

    // compile
    auto composerPass = renderGraph.CreateRenderPass("objectvis");
    composerPass->SetColor(targetBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    composerPass->SetTexture(objectBuffer);

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
        descriptor->SetTexture("Object", objectBuffer->GetImage(), sampler);
        info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

        // draw
        info.commandBuffer->Draw(6, 1, 0, 0);
    });

    return;
}

void AddLinearDepthVisPass(RenderGraph& renderGraph,
                           ResourceBundle& bundle,
                           Camera* camera,
                           RenderGraph::Resource* targetBuffer,
                           RenderGraph::Resource* depthBuffer) {

    Device* device = renderGraph.GetRenderFrame()->GetDevice();

    struct CameraInfo {
        float zNear;
        float zFar;
        float zFarRcp;
    };

    // sampler
    static Sampler* sampler = bundle.AutoRelease(new Sampler(device,
        SamplerDesc { }
            .MagFilter(VK_FILTER_NEAREST)
            .MinFilter(VK_FILTER_NEAREST)));

    // vertex shader
    static Shader* vShader = bundle.AutoRelease(new spirv::VertexShader(device, "main", "shaders/fullscreen.vert.spv"));

    // fragment shader
    static Shader* fShader = bundle.AutoRelease(new spirv::FragmentShader(device, "main", "shaders/lineardepth.frag.spv"));

    // pipeline
    static auto pipelineDesc = GraphicsPipelineDesc()
        .SetName("lineardepth")
        .SetVertexShader(vShader)
        .SetFragmentShader(fShader)
        .SetCullMode(VK_CULL_MODE_BACK_BIT)
        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout(PipelineLayoutDesc()
            .AddPushConstant("Camera", Range { 0, sizeof(CameraInfo) }, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding("Depth",   SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        );

    // ------------------------------------------------------------------------------------------------------------------

    // resources
    // NOTE: no additional resources created

    // compile
    auto composerPass = renderGraph.CreateRenderPass("lineardepth");
    composerPass->SetColor(targetBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    composerPass->SetTexture(depthBuffer);

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
        descriptor->SetTexture("Depth", depthBuffer->GetImage(), sampler);
        info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

        // push constant
        CameraInfo camInfo;
        camInfo.zNear = camera->GetNear();
        camInfo.zFar = camera->GetFar();
        camInfo.zFarRcp = 1.0 / camInfo.zFar;
        info.commandBuffer->PushConstants(pipeline->Layout(), "Camera", &camInfo);

        // draw
        info.commandBuffer->Draw(6, 1, 0, 0);
    });

    return;
}
