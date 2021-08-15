#ifndef GLTFVIEWER_SKYBOX_H
#define GLTFVIEWER_SKYBOX_H

#include <slim/slim.hpp>
#include "config.h"

using namespace slim;

struct Skybox : ReferenceCountable {
    // scene
    SmartPtr<Scene> scene;
    SmartPtr<Mesh> mesh;

    // material
    SmartPtr<Shader> vShader;
    SmartPtr<Shader> fShader;
    SmartPtr<Technique> technique;
    SmartPtr<Material> material;

    // resources
    SmartPtr<GPUImage> cubemap;
    SmartPtr<Sampler> sampler;

    Skybox(CommandBuffer* commandBuffer) {
        InitCubemap(commandBuffer);
        InitMesh(commandBuffer);
        InitMaterial(commandBuffer);
        InitScene();
    }

    void InitCubemap(CommandBuffer* commandBuffer) {
        cubemap = TextureLoader::LoadCubemap(commandBuffer,
                ToAssetPath("Skyboxes/NiagaraFalls/posx.jpg"),
                ToAssetPath("Skyboxes/NiagaraFalls/negx.jpg"),
                ToAssetPath("Skyboxes/NiagaraFalls/posy.jpg"),
                ToAssetPath("Skyboxes/NiagaraFalls/negy.jpg"),
                ToAssetPath("Skyboxes/NiagaraFalls/posz.jpg"),
                ToAssetPath("Skyboxes/NiagaraFalls/negz.jpg"));

        sampler = SlimPtr<Sampler>(commandBuffer->GetDevice(), SamplerDesc());
    }

    void InitMesh(CommandBuffer* commandBuffer) {
        Cube skyboxData = Cube { };
        skyboxData.ccw = false;
        auto skyboxMeshData = skyboxData.Create();
        mesh = SlimPtr<Mesh>();
        mesh->SetVertexAttrib(commandBuffer, skyboxMeshData.vertices, 0);
        mesh->SetIndexAttrib(commandBuffer, skyboxMeshData.indices);
        indexCount = skyboxMeshData.indices.size();
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
        material->SetTexture("Skybox", cubemap, sampler);
    }

    void InitScene() {
        manager = SlimPtr<SceneManager>();
        scene = manager->Create<Scene>("skybox");
        scene->SetDraw(mesh, material, DrawIndexed { indexCount, 1, 0, 0, 0 });
        scene->Update();
    }

private:
    SmartPtr<SceneManager> manager;
    uint32_t indexCount = 0;
};

#endif // GLTFVIEWER_SKYBOX_H
