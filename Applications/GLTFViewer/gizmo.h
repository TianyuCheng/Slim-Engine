#ifndef GLTFVIEWER_GIZMO_H
#define GLTFVIEWER_GIZMO_H

#include <slim/slim.hpp>

using namespace slim;

struct Gizmo : ReferenceCountable {
    // scene
    SmartPtr<scene::Builder> builder;
    SmartPtr<scene::Node> scene;
    SmartPtr<Mesh> cylinderMesh;
    SmartPtr<Mesh> coneMesh;

    // material
    SmartPtr<Shader> vShader;
    SmartPtr<Shader> fShader;
    SmartPtr<Technique> technique;
    SmartPtr<Material> red;
    SmartPtr<Material> green;
    SmartPtr<Material> blue;

    Gizmo(CommandBuffer* commandBuffer, scene::Builder* builder) : builder(builder) {
        InitMesh();
        InitMaterial(commandBuffer);
        InitScene();
    }

    void InitMesh() {
        GeometryData cylinder = Cylinder { 0.1f, 0.1f, 3.0f, 8, 1 }.Create();
        GeometryData cone = Cone { 0.5f, 1.0f, 16, 1 }.Create();

        // cylinder mesh
        cylinderMesh = builder->CreateMesh();
        cylinderMesh->SetIndexBuffer(cylinder.indices);
        cylinderMesh->SetVertexBuffer(cylinder.vertices);

        // cone mesh
        coneMesh = builder->CreateMesh();
        coneMesh->SetIndexBuffer(cone.indices);
        coneMesh->SetVertexBuffer(cone.vertices);
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
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("Camera",           SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT)
                    .AddBinding("Color",            SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT)
                    .AddBinding("Model",            SetBinding { 2, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
                )
        );

        red = SlimPtr<Material>(commandBuffer->GetDevice(), technique);
        red->SetUniformBuffer("Color", glm::vec3(1.0, 0.0, 0.0));

        green = SlimPtr<Material>(commandBuffer->GetDevice(), technique);
        green->SetUniformBuffer("Color", glm::vec3(0.0, 1.0, 0.0));

        blue = SlimPtr<Material>(commandBuffer->GetDevice(), technique);
        blue->SetUniformBuffer("Color", glm::vec3(0.0, 0.0, 1.0));
    }

    void InitScene() {
        scene = builder->CreateNode("gizmo");

        auto redCylinder = builder->CreateNode("red-cylinder", scene);
        auto redCone = builder->CreateNode("red-cylinder", redCylinder);
        redCylinder->SetDraw(cylinderMesh, red);
        redCone->SetDraw(coneMesh, red);
        redCylinder->Rotate(glm::vec3(0.0, 0.0, 1.0), -M_PI / 2.0);
        redCone->Translate(0.0, 3.0, 0.0);

        auto greenCylinder = builder->CreateNode("green-cylinder", scene);
        auto greenCone = builder->CreateNode("green-cylinder", greenCylinder);
        greenCylinder->SetDraw(cylinderMesh, green);
        greenCone->SetDraw(coneMesh, green);
        greenCone->Translate(0.0, 3.0, 0.0);

        auto blueCylinder = builder->CreateNode("blue-cylinder", scene);
        auto blueCone = builder->CreateNode("blue-cylinder", blueCylinder);
        blueCylinder->SetDraw(cylinderMesh, blue);
        blueCone->SetDraw(coneMesh, blue);
        blueCylinder->Rotate(glm::vec3(1.0, 0.0, 0.0), M_PI / 2.0);
        blueCone->Translate(0.0, 3.0, 0.0);

        scene->ApplyTransform();
    }
};

#endif // GLTFVIEWER_GIZMO_H
