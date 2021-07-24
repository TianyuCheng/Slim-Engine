#include <slim/slim.hpp>
#include "helper.h"

using namespace slim;

int main() {
    // create a vulkan context
    auto context = SlimPtr<Context>(
        ContextDesc()
            .EnableCompute(true)
            .EnableGraphics(true)
            .EnableValidation(true),
        WindowDesc()
            .SetResolution(640, 480)
            .SetResizable(true)
            .SetTitle("Scene Graph")
    );

    // create vertex and fragment shaders
    auto vShader = SlimPtr<spirv::VertexShader>(context, "main", "shaders/simple.vert.spv");
    auto fShader = SlimPtr<spirv::FragmentShader>(context, "main", "shaders/simple.frag.spv");

    // create technique
    auto technique = SlimPtr<Technique>();
    technique->AddPass(RenderQueue::Opaque,
        GraphicsPipelineDesc()
            .SetName("textured")
            .AddVertexBinding(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
                { 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) },
             })
            .SetVertexShader(vShader)
            .SetFragmentShader(fShader)
            .SetCullMode(VK_CULL_MODE_BACK_BIT)
            .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .SetDepthTest(VK_COMPARE_OP_LESS)
            .SetPipelineLayout(PipelineLayoutDesc()
                .AddPushConstant("Xform", 0, sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
                .AddBinding("Camera", 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("Color",  1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)));

    // create the first material
    auto material1 = SlimPtr<Material>(context, technique);
    material1->SetUniform("Color", glm::vec3(1.0f, 1.0f, 0.0f));

    // create the second material
    auto material2 = SlimPtr<Material>(context, technique);
    material2->SetUniform("Color", glm::vec3(0.0f, 1.0f, 1.0f));

    // create mesh and its submeshes
    auto mesh = SlimPtr<Mesh>(CreateMesh(context));
    auto submesh = Submesh(mesh, 0, 0, 36);

    // create a scene
    auto scene = SlimPtr<SceneNode>("scene");
    auto child1 = SlimPtr<SceneNode>("child1", scene);
    auto child2 = SlimPtr<SceneNode>("child2", scene);
    auto child11 = SlimPtr<SceneNode>("child1.1", child1);
    auto child21 = SlimPtr<SceneNode>("child2.1", child2);
    {
        child1->SetMesh(submesh);
        child1->SetMaterial(material1);
        child1->Translate(1.0f, 0.0f, 0.0f);

        child11->SetMesh(submesh);
        child11->SetMaterial(material1);
        child11->Translate(0.0f, 1.0f, 0.0f);
        child11->Scale(0.5f, 0.5f, 0.5f);

        child2->SetMesh(submesh);
        child2->SetMaterial(material2);
        child2->Translate(-1.0f, 0.0f, 0.0f);

        child21->SetMesh(submesh);
        child21->SetMaterial(material2);
        child21->Translate(0.0f, 1.0f, 0.0f);
        child21->Scale(0.5f, 0.5f, 0.5f);
    }
    scene->Init();

    // render
    auto window = context->GetWindow();
    while (!window->ShouldClose()) {
        // query image from swapchain
        auto frame = window->AcquireNext();

        // create a camera
        Camera camera("camera");
        camera.LookAt(glm::vec3(0.0, 0.0, 3.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
        camera.Perspective(1.05, frame->GetAspectRatio(), 0.1, 20.0f);

        // transform scene nodes
        child1->Rotate(glm::vec3(0.0f, 1.0f, 0.0f), +0.025);
        child2->Rotate(glm::vec3(0.0f, 1.0f, 0.0f), -0.025);
        scene->Update();

        // scenegraph for rendering organization
        auto sceneGraph = SlimPtr<SceneGraph>(scene);
        sceneGraph->Cull(camera);

        // rendergraph-based design
        RenderGraph renderGraph(frame);
        {
            auto colorBuffer = renderGraph.CreateResource(frame->GetBackBuffer());
            auto depthBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_D24_UNORM_S8_UINT, VK_SAMPLE_COUNT_1_BIT);

            auto colorPass = renderGraph.CreateRenderPass("color");
            colorPass->SetColor(colorBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            colorPass->SetDepthStencil(depthBuffer, ClearValue(1.0f, 0));
            colorPass->Execute([=](const RenderGraph &renderGraph) {
                sceneGraph->Render(renderGraph, camera, RenderQueue::Opaque, SortingOrder::FrontToback);
            });
        }
        renderGraph.Execute();

        // window update
        window->PollEvents();
    }

    context->WaitIdle();
    return EXIT_SUCCESS;
}
