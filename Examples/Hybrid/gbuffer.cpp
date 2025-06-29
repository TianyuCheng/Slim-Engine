#include "scene.h"
#include "gbuffer.h"

void AddGBufferPass(RenderGraph& renderGraph,
                    ResourceBundle& bundle,
                    MainScene* scene,
                    GBuffer* gbuffer) {

    Device* device = renderGraph.GetRenderFrame()->GetDevice();

    // vertex shader
    static Shader* vShader = bundle.AutoRelease(new spirv::VertexShader(device, "shaders/gbuffer.vert.spv"));

    // fragment shader
    static Shader* fShader = bundle.AutoRelease(new spirv::FragmentShader(device, "shaders/gbuffer.frag.spv"));

    // pipeline
    VkDescriptorBindingFlags bindFlags = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
                                       | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    static auto pipelineDesc = GraphicsPipelineDesc()
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
        .SetDefaultBlendState(3)
        .SetPipelineLayout(PipelineLayoutDesc()
            .AddPushConstant("Object",     Range { 0, sizeof(ObjectProperties) },   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding     ("Camera",     SetBinding { 0, 0 },                     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .AddBinding     ("Transforms", SetBinding { 0, 1 },                     VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .AddBindingArray("Images",     SetBinding { 1, 0 }, 1000,               VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  VK_SHADER_STAGE_FRAGMENT_BIT, bindFlags)
            .AddBindingArray("Samplers",   SetBinding { 2, 0 }, 1000,               VK_DESCRIPTOR_TYPE_SAMPLER,        VK_SHADER_STAGE_FRAGMENT_BIT, bindFlags)
        );

    // ------------------------------------------------------------------------------------------------------------------

    // compile
    auto gbufferPass = renderGraph.CreateRenderPass("gbuffer");
    gbufferPass->SetColor(gbuffer->albedoBuffer,   ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    gbufferPass->SetColor(gbuffer->normalBuffer,   ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    gbufferPass->SetColor(gbuffer->positionBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    gbufferPass->SetColor(gbuffer->objectBuffer,   ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    gbufferPass->SetDepthStencil(gbuffer->depthBuffer, ClearValue(1.0f, 0));
    gbufferPass->SetStorage(scene->lightResource, RenderGraph::STORAGE_READ_ONLY);
    gbufferPass->SetStorage(scene->cameraResource, RenderGraph::STORAGE_READ_ONLY);

    // execute
    gbufferPass->Execute([=](const RenderInfo& info) {

        // bind pipeline
        auto pipeline = info.renderFrame->RequestPipeline(
            pipelineDesc
                .SetRenderPass(info.renderPass)
                .SetViewport(info.renderFrame->GetExtent()));
        info.commandBuffer->BindPipeline(pipeline);

        // bind camera uniform
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetUniformBuffer("Camera", scene->cameraBuffer);
        descriptor->SetStorageBuffer("Transforms", scene->transformBuffer);
        descriptor->SetSampledImages("Images", scene->images);
        descriptor->SetSamplers("Samplers", scene->samplers);
        info.commandBuffer->BindDescriptor(descriptor, VK_PIPELINE_BIND_POINT_GRAPHICS);

        // draw each object instance
        scene->builder->ForEachInstance([&](scene::Node*, scene::Mesh* mesh, scene::Material* material, uint32_t instanceID) {
            gltf::MaterialData& data = material->GetData<gltf::MaterialData>();

            ObjectProperties properties = {};
            properties.instanceID = instanceID;
            properties.baseColorSamplerID = data.baseColorSampler;
            properties.baseColorTextureID = data.baseColorTexture;

            mesh->Bind(info.commandBuffer);
            info.commandBuffer->PushConstants(pipeline->Layout(), "Object", &properties);
            info.commandBuffer->DrawIndexed(mesh->GetIndexCount(), 1, 0, 0, 0);
        });

    });
}
