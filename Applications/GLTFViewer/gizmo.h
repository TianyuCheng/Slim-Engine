#ifndef GLTFVIEWER_GIZMO_H
#define GLTFVIEWER_GIZMO_H

#include <slim/slim.hpp>
#include "config.h"

using namespace slim;

struct Gizmo {
    // scene
    SmartPtr<Scene> scene;
    SmartPtr<Mesh> mesh;

    // material
    SmartPtr<Shader> vShader;
    SmartPtr<Shader> fShader;
    SmartPtr<Technique> technique;
    SmartPtr<Material> red;
    SmartPtr<Material> green;
    SmartPtr<Material> blue;

    Gizmo() {

    }

    void Init(CommandBuffer* commandBuffer) {
        InitMesh(commandBuffer);
        InitMaterial(commandBuffer);
        InitScene();
    }

    void InitMesh(CommandBuffer* commandBuffer) {
        GeometryData cylinder = Cylinder { 0.5f, 0.5f, 3.0f, 8, 1 }.Create();
        GeometryData cone = Cone { 1.0f, 1.0f, 8, 1 }.Create();

        cylinderIndexCount = cylinder.indices.size();
        cylinderVertexCount = cylinder.vertices.size();

        coneIndexCount = cone.indices.size();
        coneVertexCount = cone.vertices.size();

        // copy vertices
        std::vector<GeometryData::Vertex> vertices;
        vertices.reserve(cylinder.vertices.size() + cone.vertices.size());
        std::copy(cylinder.vertices.begin(), cylinder.vertices.end(), std::back_inserter(vertices));
        std::copy(cone.vertices.begin(), cone.vertices.end(), std::back_inserter(vertices));

        // copy indices
        std::vector<uint32_t> indices;
        indices.reserve(cylinder.indices.size() + cone.indices.size());
        std::copy(cylinder.indices.begin(), cylinder.indices.end(), std::back_inserter(indices));
        std::copy(cone.indices.begin(), cone.indices.end(), std::back_inserter(indices));

        // create mesh
        mesh = SlimPtr<Mesh>();
        mesh->SetIndexAttrib(commandBuffer, indices);
        mesh->SetVertexAttrib(commandBuffer, vertices, 0);
    }

    void InitMaterial(CommandBuffer* commandBuffer) {
        vShader = new spirv::VertexShader(commandBuffer->GetDevice(), "main", "shaders/gizmo.vert.spv");
        fShader = new spirv::FragmentShader(commandBuffer->GetDevice(), "main", "shaders/gizmo.frag.spv");

        technique = SlimPtr<Technique>();
        technique->AddPass(
            RenderQueue::Opaque,
            GraphicsPipelineDesc()
                .SetName("Gizmo")
                .AddVertexBinding(0, sizeof(GeometryData::Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
                    { 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(GeometryData::Vertex, position)      },
                    { 1, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(GeometryData::Vertex, normal)        },
                 })
                .SetVertexShader(vShader)
                .SetFragmentShader(fShader)
                .SetCullMode(VK_CULL_MODE_BACK_BIT)
                .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .SetDepthTest(VK_COMPARE_OP_LESS_OR_EQUAL)
                .SetSampleCount(msaa)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("Camera",           0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT)
                    .AddBinding("Color",            1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT)
                    .AddBinding("Model",            2, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
                )
        );

        red = SlimPtr<Material>(commandBuffer->GetDevice(), technique);
        red->SetUniform("Color", glm::vec3(1.0, 0.0, 0.0));

        green = SlimPtr<Material>(commandBuffer->GetDevice(), technique);
        green->SetUniform("Color", glm::vec3(0.0, 1.0, 0.0));

        blue = SlimPtr<Material>(commandBuffer->GetDevice(), technique);
        blue->SetUniform("Color", glm::vec3(0.0, 0.0, 1.0));
    }

    void InitScene() {
        manager = SlimPtr<SceneManager>();
        scene = manager->Create<Scene>("gizmo");
        scene->Scale(0.1, 0.1, 0.1);

        auto redCylinder = manager->Create<Scene>("red-cylinder", scene);
        auto redCone = manager->Create<Scene>("red-cylinder", redCylinder);
        redCylinder->SetDraw(mesh, red, DrawIndexed {
            cylinderIndexCount, 1, 0, 0, 0
        });
        redCone->SetDraw(mesh, red, DrawIndexed {
            coneIndexCount, 1, cylinderIndexCount, static_cast<int32_t>(cylinderVertexCount), 0
        });
        redCylinder->Rotate(glm::vec3(0.0, 0.0, 1.0), -M_PI / 2.0);
        redCone->Translate(0.0, 3.0, 0.0);

        auto greenCylinder = manager->Create<Scene>("green-cylinder", scene);
        auto greenCone = manager->Create<Scene>("green-cylinder", greenCylinder);
        greenCylinder->SetDraw(mesh, green, DrawIndexed { cylinderIndexCount, 1, 0, 0, 0 });
        greenCone->SetDraw(mesh, green, DrawIndexed { coneIndexCount, 1, cylinderIndexCount, static_cast<int32_t>(cylinderVertexCount), 0 });
        greenCone->Translate(0.0, 3.0, 0.0);

        auto blueCylinder = manager->Create<Scene>("blue-cylinder", scene);
        auto blueCone = manager->Create<Scene>("blue-cylinder", blueCylinder);
        blueCylinder->SetDraw(mesh, blue, DrawIndexed { cylinderIndexCount, 1, 0, 0, 0 });
        blueCone->SetDraw(mesh, blue, DrawIndexed { coneIndexCount, 1, cylinderIndexCount, static_cast<int32_t>(cylinderVertexCount), 0 });
        blueCylinder->Rotate(glm::vec3(1.0, 0.0, 0.0), M_PI / 2.0);
        blueCone->Translate(0.0, 3.0, 0.0);

        scene->Update();
    }

private:
    uint32_t cylinderIndexCount = 0;
    uint32_t cylinderVertexCount = 0;
    uint32_t coneIndexCount = 0;
    uint32_t coneVertexCount = 0;
    SmartPtr<SceneManager> manager;
};

#endif // GLTFVIEWER_GIZMO_H
