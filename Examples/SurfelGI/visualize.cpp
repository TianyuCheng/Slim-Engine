#include "composer.h"

void AddObjectVisPass(RenderGraph& renderGraph,
                      AutoReleasePool& pool,
                      RenderGraph::Resource* targetBuffer,
                      RenderGraph::Resource* objectBuffer) {

    // sampler
    static auto sampler = pool.FetchOrCreate(
        "nearest.sampler",
        [&](Device* device) {
            return new Sampler(device,
                SamplerDesc { }
                    .MagFilter(VK_FILTER_NEAREST)
                    .MinFilter(VK_FILTER_NEAREST));
        });

    // vertex shader
    static auto vShader = pool.FetchOrCreate(
        "fullscreen.vertex.shader",
        [&](Device* device) {
            return new spirv::VertexShader(device, "main", "shaders/fullscreen.vert.spv");
        });

    // fragment shader
    static auto fShader = pool.FetchOrCreate(
        "object.visualize.fragment.shader",
        [&](Device* device) {
            return new spirv::FragmentShader(device, "main", "shaders/objectvis.frag.spv");
        });

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
                           AutoReleasePool& pool,
                           MainScene* scene,
                           RenderGraph::Resource* targetBuffer,
                           RenderGraph::Resource* depthBuffer) {

    // sampler
    static auto sampler = pool.FetchOrCreate(
        "nearest.sampler",
        [&](Device* device) {
            return new Sampler(device,
                SamplerDesc { }
                    .MagFilter(VK_FILTER_NEAREST)
                    .MinFilter(VK_FILTER_NEAREST));
        });

    // vertex shader
    static auto vShader = pool.FetchOrCreate(
        "fullscreen.vertex.shader",
        [&](Device* device) {
            return new spirv::VertexShader(device, "main", "shaders/fullscreen.vert.spv");
        });

    // fragment shader
    static auto fShader = pool.FetchOrCreate(
        "linear.depth.fragment.shader",
        [&](Device* device) {
            return new spirv::FragmentShader(device, "main", "shaders/lineardepth.frag.spv");
        });

    // pipeline
    static auto pipelineDesc = GraphicsPipelineDesc()
        .SetName("lineardepth")
        .SetVertexShader(vShader)
        .SetFragmentShader(fShader)
        .SetCullMode(VK_CULL_MODE_BACK_BIT)
        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout(PipelineLayoutDesc()
            .AddBinding("Depth",   SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding("Camera",  SetBinding { 0, 1 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT)
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
        descriptor->SetUniformBuffer("Camera", scene->cameraBuffer);
        info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

        // draw
        info.commandBuffer->Draw(6, 1, 0, 0);
    });

    return;
}

void AddSurfelAllocVisPass(RenderGraph& renderGraph,
                           AutoReleasePool& pool,
                           Buffer* buffer,
                           RenderGraph::Resource* targetBuffer) {

    // vertex shader
    static auto vShader = pool.FetchOrCreate(
        "fullscreen.vertex.shader",
        [&](Device* device) {
            return new spirv::VertexShader(device, "main", "shaders/fullscreen.vert.spv");
        });

    // fragment shader
    static auto fShader = pool.FetchOrCreate(
        "surfel.alloc.visualize.fragment.shader",
        [&](Device* device) {
            return new spirv::FragmentShader(device, "main", "shaders/surfelallocvis.frag.spv");
        });

    // pipeline
    static auto pipelineDesc = GraphicsPipelineDesc()
        .SetName("surfel-alloc-vis")
        .SetVertexShader(vShader)
        .SetFragmentShader(fShader)
        .SetCullMode(VK_CULL_MODE_BACK_BIT)
        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout(PipelineLayoutDesc()
            .AddBinding("SurfelStat", SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        );

    // ------------------------------------------------------------------------------------------------------------------

    // resources
    // NOTE: no additional resources created

    // compile
    auto surfelAllocVisPass = renderGraph.CreateRenderPass("surfel-alloc-vis");
    surfelAllocVisPass->SetColor(targetBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));

    // execute
    surfelAllocVisPass->Execute([=](const RenderInfo& info) {
        auto pipeline = info.renderFrame->RequestPipeline(
            pipelineDesc
                .SetViewport(info.renderFrame->GetExtent())
                .SetRenderPass(info.renderPass)
        );

        // bind pipeline
        info.commandBuffer->BindPipeline(pipeline);

        // bind descriptor
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetStorageBuffer("SurfelStat", buffer);
        info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

        // draw
        info.commandBuffer->Draw(6, 1, 0, 0);
    });

    return;
}

void AddSurfelGridVisPass(RenderGraph& renderGraph,
                          AutoReleasePool& pool,
                          MainScene* scene,
                          RenderGraph::Resource* targetBuffer,
                          RenderGraph::Resource* depthBuffer) {

    struct FrameInfo {
        glm::uvec2 size;
    };

    // sampler
    static auto sampler = pool.FetchOrCreate(
        "nearest.sampler",
        [&](Device* device) {
            return new Sampler(device,
                SamplerDesc { }
                    .MagFilter(VK_FILTER_NEAREST)
                    .MinFilter(VK_FILTER_NEAREST));
        });

    // vertex shader
    static auto vShader = pool.FetchOrCreate(
        "fullscreen.vertex.shader",
        [&](Device* device) {
            return new spirv::VertexShader(device, "main", "shaders/fullscreen.vert.spv");
        });

    // fragment shader
    static auto fShader = pool.FetchOrCreate(
        "surfel.grid.fragment.shader",
        [&](Device* device) {
            return new spirv::FragmentShader(device, "main", "shaders/surfelgridvis.frag.spv");
        });

    // pipeline
    static auto pipelineDesc = GraphicsPipelineDesc()
        .SetName("surfel-grid-vis")
        .SetVertexShader(vShader)
        .SetFragmentShader(fShader)
        .SetCullMode(VK_CULL_MODE_BACK_BIT)
        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout(PipelineLayoutDesc()
            .AddPushConstant("Info", Range { 0, sizeof(FrameInfo) }, VK_SHADER_STAGE_FRAGMENT_BIT)
            // set 0
            .AddBinding("Camera",   SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding("Depth",    SetBinding { 0, 1 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        );

    // ------------------------------------------------------------------------------------------------------------------

    // resources
    // NOTE: no additional resources created

    // compile
    auto surfelGridVisPass = renderGraph.CreateRenderPass("surfel-grid-vis");
    surfelGridVisPass->SetColor(targetBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    surfelGridVisPass->SetTexture(depthBuffer);

    // execute
    surfelGridVisPass->Execute([=](const RenderInfo& info) {
        auto pipeline = info.renderFrame->RequestPipeline(
            pipelineDesc
                .SetViewport(info.renderFrame->GetExtent())
                .SetRenderPass(info.renderPass)
        );

        // bind pipeline
        info.commandBuffer->BindPipeline(pipeline);

        // bind descriptor
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetUniformBuffer("Camera", scene->cameraBuffer);
        descriptor->SetTexture("Depth", depthBuffer->GetImage(), sampler);
        info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

        // push constant
        FrameInfo frameInfo = {};
        frameInfo.size.x = info.renderFrame->GetExtent().width;
        frameInfo.size.y = info.renderFrame->GetExtent().height;
        info.commandBuffer->PushConstants(pipeline->Layout(), "Info", &frameInfo);

        // draw
        info.commandBuffer->Draw(6, 1, 0, 0);
    });

    return;
}
