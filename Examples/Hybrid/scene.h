#include <slim/slim.hpp>
#include "light.h"

using namespace slim;

class MainScene {
public:
    MainScene(Device* device) : device(device) {
        PrepareTechnique();
        PrepareRTPipeline();
        PreparePostPipeline();
        PrepareScene();
    }

    void PrepareTechnique() {
        // create vertex and fragment shaders
        vShader = SlimPtr<spirv::VertexShader>(device, "main", "shaders/gbuffer.vert.spv");
        fShader = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/gbuffer.frag.spv");

        // create technique
        technique = SlimPtr<Technique>();
        technique->AddPass(RenderQueue::Opaque,
            GraphicsPipelineDesc()
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
                    .AddBinding("Camera",    SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT)
                    .AddBinding("Model",     SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
                    .AddBinding("MainTex",   SetBinding { 2, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                ));
    }

    void PrepareRTPipeline() {
        // create rt shaders
        shadowRayGenShader = SlimPtr<spirv::RayGenShader>(device, "main", "shaders/shadow.rgen.spv");
        shadowRayMissShader = SlimPtr<spirv::MissShader>(device, "main", "shaders/shadow.rmiss.spv");
        shadowRayClosestHitShader = SlimPtr<spirv::ClosestHitShader>(device, "main", "shaders/shadow.rchit.spv");

        // create pipeline
        rtPipeline = SlimPtr<Pipeline>(
            device,
            RayTracingPipelineDesc()
                .SetName("hybrid-raytracing")
                .SetRayGenShader(shadowRayGenShader)
                .SetMissShader(shadowRayMissShader)
                .SetClosestHitShader(shadowRayClosestHitShader)
                .SetMaxRayRecursionDepth(1)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddPushConstant("DirectionalLight", Range { 0, sizeof(DirectionalLight) }, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                    .AddBinding("Accel",    SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                    .AddBinding("Albedo",   SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                    .AddBinding("Normal",   SetBinding { 1, 1 }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                    .AddBinding("Position", SetBinding { 1, 2 }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                )
        );
    }

    void PreparePostPipeline() {
        // create vertex and fragment shaders
        vPostShader = SlimPtr<spirv::VertexShader>(device, "main", "shaders/compose.vert.spv");
        fPostShader = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/compose.frag.spv");

        // create sampler
        sampler = SlimPtr<Sampler>(device, SamplerDesc { });
    }

    void PrepareScene() {
        // model loading
        builder = SlimPtr<scene::Builder>(device);

        // enable ray tracing builder and acceleration structure compaction
        builder->EnableRayTracing();
        builder->GetAccelBuilder()->EnableCompaction();

        // load model
        model.Load(builder, ToAssetPath("Scenes/Sponza/glTF/Sponza.gltf"));

        // update materials
        for (auto& material : model.materials) {
            material->SetTechnique(technique);

            // we only need base color texture
            const gltf::MaterialData& data = material->GetData<gltf::MaterialData>();
            material->SetTexture("MainTex", model.images[data.baseColorTexture], model.samplers[data.baseColorSampler]);
        }

        // create scene
        builder->Build();
    }

    void Draw(const RenderInfo& info, Camera* camera) {
        // sceneFilter result + sorting
        auto culling = CPUCulling();
        culling.Cull(model.GetScene(0), camera);
        culling.Sort(RenderQueue::Geometry,    RenderQueue::GeometryLast, SortingOrder::FrontToback);
        culling.Sort(RenderQueue::Transparent, RenderQueue::Transparent,  SortingOrder::BackToFront);
        // rendering
        MeshRenderer renderer(info);
        renderer.Draw(camera, culling.GetDrawables(RenderQueue::Geometry, RenderQueue::GeometryLast));
    }

    void RayTrace(const RenderInfo& info,
                  const DirectionalLight& dirLight,
                  Image* albedo, Image* normal, Image* position) {
        CommandBuffer* commandBuffer = info.commandBuffer;
        RenderFrame* renderFrame = info.renderFrame;

        // bind pipeline
        commandBuffer->BindPipeline(rtPipeline);

        // bind descriptor
        auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), rtPipeline->Layout());
        descriptor->SetAccelStruct("Accel", builder->GetAccelBuilder()->GetTlas());
        descriptor->SetStorageImage("Albedo", albedo);
        descriptor->SetStorageImage("Normal", normal);
        descriptor->SetStorageImage("Position", position);
        commandBuffer->BindDescriptor(descriptor, rtPipeline->Type());
        commandBuffer->PushConstants(rtPipeline->Layout(), "DirectionalLight", &dirLight);

        // trace rays
        const VkExtent3D& extent = position->GetExtent();
        vkCmdTraceRaysKHR(*commandBuffer,
                          rtPipeline->GetRayGenRegion(),
                          rtPipeline->GetMissRegion(),
                          rtPipeline->GetHitRegion(),
                          rtPipeline->GetCallableRegion(),
                          extent.width, extent.height, 1);
    }

    void Compose(const RenderInfo& info,
                 const DirectionalLight& dirLight,
                 Image* albedo, Image* normal, Image* position) {
        CommandBuffer* commandBuffer = info.commandBuffer;
        RenderFrame* renderFrame = info.renderFrame;

        auto pipeline = renderFrame->RequestPipeline(
            GraphicsPipelineDesc()
                .SetName("composer")
                .SetVertexShader(vPostShader)
                .SetFragmentShader(fPostShader)
                .SetCullMode(VK_CULL_MODE_BACK_BIT)
                .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .SetRenderPass(info.renderPass)
                .SetViewport(renderFrame->GetExtent())
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddPushConstant("DirectionalLight", Range { 0, sizeof(DirectionalLight) }, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .AddBinding("Albedo",   SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .AddBinding("Normal",   SetBinding { 0, 1 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .AddBinding("Position", SetBinding { 0, 2 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                )
        );

        // bind pipeline
        commandBuffer->BindPipeline(pipeline);

        // bind descriptor
        auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetTexture("Albedo", albedo, sampler);
        descriptor->SetTexture("Normal", normal, sampler);
        descriptor->SetTexture("Position", position, sampler);
        commandBuffer->BindDescriptor(descriptor, pipeline->Type());
        commandBuffer->PushConstants(pipeline->Layout(), "DirectionalLight", &dirLight);

        // draw
        vkCmdDraw(*commandBuffer, 6, 1, 0, 0);
    }

    SmartPtr<Device>                  device;

    // rasterizer
    SmartPtr<Technique>               technique;
    SmartPtr<spirv::VertexShader>     vShader;
    SmartPtr<spirv::FragmentShader>   fShader;

    // ray tracing
    SmartPtr<Pipeline>                rtPipeline;
    SmartPtr<spirv::RayGenShader>     shadowRayGenShader;
    SmartPtr<spirv::MissShader>       shadowRayMissShader;
    SmartPtr<spirv::ClosestHitShader> shadowRayClosestHitShader;

    // compose
    GraphicsPipelineDesc              postPipelineDesc;
    SmartPtr<spirv::VertexShader>     vPostShader;
    SmartPtr<spirv::FragmentShader>   fPostShader;
    SmartPtr<Sampler>                 sampler;

    // scene/models
    SmartPtr<scene::Builder>          builder;
    gltf::Model                       model;
};
