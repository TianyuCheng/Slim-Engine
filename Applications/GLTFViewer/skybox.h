#ifndef GLTFVIEWER_SKYBOX_H
#define GLTFVIEWER_SKYBOX_H

#include <slim/slim.hpp>
#include "config.h"

using namespace slim;

struct Skybox : ReferenceCountable {
    // scene
    SmartPtr<scene::Builder> builder;
    SmartPtr<scene::Node> scene;
    SmartPtr<Mesh> mesh;

    // material
    SmartPtr<Shader> vShader;
    SmartPtr<Shader> fShader;
    SmartPtr<Technique> technique;
    SmartPtr<Material> material;

    // resources
    SmartPtr<GPUImage> skybox;
    SmartPtr<GPUImage> irradiance;
    SmartPtr<Sampler> sampler;

    Skybox(CommandBuffer* commandBuffer, scene::Builder* builder) : builder(builder) {
        InitCubemap(commandBuffer);
        InitMaterial(commandBuffer);
        InitScene();
    }

    void InitCubemap(CommandBuffer* commandBuffer) {
        skybox = TextureLoader::LoadCubemap(commandBuffer,
                ToAssetPath("Skyboxes/Ridgecrest_Road/cubemap/px.png"),
                ToAssetPath("Skyboxes/Ridgecrest_Road/cubemap/nx.png"),
                ToAssetPath("Skyboxes/Ridgecrest_Road/cubemap/py.png"),
                ToAssetPath("Skyboxes/Ridgecrest_Road/cubemap/ny.png"),
                ToAssetPath("Skyboxes/Ridgecrest_Road/cubemap/pz.png"),
                ToAssetPath("Skyboxes/Ridgecrest_Road/cubemap/nz.png"));

        irradiance = TextureLoader::LoadCubemap(commandBuffer,
                ToAssetPath("Skyboxes/Ridgecrest_Road/irradiance/px.png"),
                ToAssetPath("Skyboxes/Ridgecrest_Road/irradiance/nx.png"),
                ToAssetPath("Skyboxes/Ridgecrest_Road/irradiance/py.png"),
                ToAssetPath("Skyboxes/Ridgecrest_Road/irradiance/ny.png"),
                ToAssetPath("Skyboxes/Ridgecrest_Road/irradiance/pz.png"),
                ToAssetPath("Skyboxes/Ridgecrest_Road/irradiance/nz.png"));

        sampler = SlimPtr<Sampler>(commandBuffer->GetDevice(), SamplerDesc());
    }

    void InitMaterial(CommandBuffer* commandBuffer) {
        auto vShader = SlimPtr<spirv::VertexShader>(commandBuffer->GetDevice(), "main", "shaders/skybox.vert.spv");
        auto fShader = SlimPtr<spirv::FragmentShader>(commandBuffer->GetDevice(), "main", "shaders/skybox.frag.spv");

        auto technique = SlimPtr<Technique>();
        technique->AddPass(RenderQueue::Opaque,
            GraphicsPipelineDesc()
                .SetName("skybox")
                .AddVertexBinding(0, sizeof(GeometryData::Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
                    { 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryData::Vertex, position) },
                 })
                .SetVertexShader(vShader)
                .SetFragmentShader(fShader)
                .SetCullMode(VK_CULL_MODE_BACK_BIT)
                .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .SetDepthTest(VK_COMPARE_OP_ALWAYS, false)
                .SetSampleCount(msaa)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("Camera", 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT)
                    .AddBinding("Skybox", 1, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .AddBinding("Model",  2, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)));


        material = SlimPtr<Material>(commandBuffer->GetDevice(), technique);
        material->SetTexture("Skybox", skybox, sampler);
    }

    void InitScene() {
        Cube skyboxData = Cube { };
        skyboxData.ccw = false;
        auto skyboxMeshData = skyboxData.Create();
        mesh = builder->CreateMesh();
        mesh->SetVertexBuffer(skyboxMeshData.vertices);
        mesh->SetIndexBuffer(skyboxMeshData.indices);
        mesh->AddInputBinding(0, 0);

        scene = builder->CreateNode("skybox");
        scene->SetDraw(mesh, material);
        scene->ApplyTransform();
    }
};

#endif // GLTFVIEWER_SKYBOX_H
