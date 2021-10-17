#include "gbuffer.h"
#include "shaders/common.h"

GraphicsPipelineDesc PrepareGBufferPass(AutoReleasePool& pool) {
    // vertex shader
    auto vShader = pool.FetchOrCreate(
        "gbuffer.vertex.shader",
        [](Device* device) {
            return new spirv::VertexShader(device, "main", "shaders/gbuffer/gbuffer.vert.spv");
        });

    // fragment shader
    auto fShader = pool.FetchOrCreate(
        "gbuffer.fragment.shader",
        [](Device* device) {
            return new spirv::FragmentShader(device, "main", "shaders/gbuffer/gbuffer.frag.spv");
        });

    // pipeline
    VkDescriptorBindingFlags bindFlags = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
                                       | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    auto pipelineDesc = GraphicsPipelineDesc()
        .SetName("gbuffer")
        .AddVertexBinding(0, sizeof(gltf::Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
            { 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(gltf::Vertex, position)) },
            { 1, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(gltf::Vertex, normal  )) },
            { 2, VK_FORMAT_R32G32_SFLOAT,    static_cast<uint32_t>(offsetof(gltf::Vertex, uv0     )) },
         })
        .SetVertexShader(vShader)
        .SetFragmentShader(fShader)
        .SetCullMode(VK_CULL_MODE_BACK_BIT)
        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetDepthTest(VK_COMPARE_OP_LESS)
        .SetDefaultBlendState(0)
        .SetDefaultBlendState(1)
        .SetDefaultBlendState(2)
        .SetPipelineLayout(PipelineLayoutDesc()
            .AddPushConstant("InstanceID", Range      { 0, sizeof(uint32_t)       },                                          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding     ("Camera",     SetBinding { 0, SCENE_CAMERA_BINDING   },       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .AddBinding     ("Instances",  SetBinding { 0, SCENE_INSTANCE_BINDING },       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding     ("Materials",  SetBinding { 0, SCENE_MATERIAL_BINDING },       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBindingArray("Images",     SetBinding { 1, SCENE_IMAGES_BINDING   }, 1000, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  VK_SHADER_STAGE_FRAGMENT_BIT, bindFlags)
            .AddBindingArray("Samplers",   SetBinding { 2, SCENE_SAMPLERS_BINDING }, 1000, VK_DESCRIPTOR_TYPE_SAMPLER,        VK_SHADER_STAGE_FRAGMENT_BIT, bindFlags)
        );

    return pipelineDesc;
}

void AddGBufferPass(RenderGraph&       graph,
                    AutoReleasePool&   pool,
                    render::GBuffer*   gbuffer,
                    render::SceneData* sceneData,
                    render::Surfel*    surfel,
                    render::Debug*     debug,
                    Scene*             scene) {

    static auto pipelineLayout = PrepareGBufferPass(pool);

    auto pass = graph.CreateRenderPass("gbuffer");
    pass->SetColor(gbuffer->albedo, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    pass->SetColor(gbuffer->normal, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    pass->SetColor(gbuffer->object, ClearValue(0.0f, 0.0f, 0.0f, 0.0f));
    pass->SetDepth(gbuffer->depth, ClearValue(1.0f, 0));
    pass->SetStorage(sceneData->camera, RenderGraph::STORAGE_READ_ONLY);
    pass->Execute([=](const RenderInfo &info) {

        // bind pipeline
        auto pipeline = info.renderFrame->RequestPipeline(
            pipelineLayout
                .SetRenderPass(info.renderPass)
                .SetViewport(info.renderFrame->GetExtent()));

        info.commandBuffer->BindPipeline(pipeline);

        // bind camera uniform
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetUniformBuffer("Camera", scene->cameraBuffer);
        descriptor->SetStorageBuffer("Instances", scene->instanceBuffer);
        descriptor->SetStorageBuffer("Materials", scene->materialBuffer);
        descriptor->SetSampledImages("Images", scene->images);
        descriptor->SetSamplers("Samplers", scene->samplers);
        info.commandBuffer->BindDescriptor(descriptor, VK_PIPELINE_BIND_POINT_GRAPHICS);

        // draw each instance
        scene->builder->ForEachInstance([&](scene::Node*, scene::Mesh* mesh, scene::Material*, uint32_t instanceID) {
            mesh->Bind(info.commandBuffer);
            info.commandBuffer->PushConstants(pipeline->Layout(), "InstanceID", &instanceID);
            info.commandBuffer->DrawIndexed(mesh->GetIndexCount(), 1, 0, 0, 0);
        });

    });
}
