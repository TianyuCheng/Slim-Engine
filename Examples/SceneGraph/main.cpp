#include <slim/slim.hpp>
#include "helper.h"

using namespace slim;

int main() {
    // create a slim device
    auto context = SlimPtr<Context>(
        ContextDesc()
            .EnableCompute(true)
            .EnableGraphics(true)
            .EnableValidation(true)
            .EnableGLFW(true)
    );

    // create a slim device
    auto device = SlimPtr<Device>(context);

    // create a slim window
    auto window = SlimPtr<Window>(
        device,
        WindowDesc()
            .SetResolution(640, 480)
            .SetResizable(true)
            .SetTitle("Scene Graph + Material System")
    );

    // create vertex and fragment shaders
    auto vShader = SlimPtr<spirv::VertexShader>(device, "main", "shaders/simple.vert.spv");
    auto fShader = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/simple.frag.spv");

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
                .AddBinding("Camera", 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("Model",  1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("Color",  2, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT)));

    // create the first material
    auto material1 = SlimPtr<Material>(device, technique);
    material1->SetUniform("Color", glm::vec3(1.0f, 1.0f, 0.0f));

    // create the second material
    auto material2 = SlimPtr<Material>(device, technique);
    material2->SetUniform("Color", glm::vec3(0.0f, 1.0f, 1.0f));

    // create mesh and its submeshes
    auto mesh = SlimPtr<Mesh>(CreateMesh(device));

    // create scene
    auto sceneMgr = SlimPtr<SceneManager>();
    auto scene    = sceneMgr->Create<Scene>("scene");
    auto scene1   = sceneMgr->Create<Scene>("child0.1", scene);
    auto scene11  = sceneMgr->Create<Scene>("child1.1", scene1);
    auto scene2   = sceneMgr->Create<Scene>("child0.2", scene);
    auto scene21  = sceneMgr->Create<Scene>("child2.1", scene2);
    {
        scene1->SetDraw(mesh, material1, DrawIndexed { 36, 1, 0, 0, 0 });
        scene1->Translate(1.0f, 0.0f, 0.0f);

        scene11->SetDraw(mesh, material1, DrawIndexed { 36, 1, 0, 0, 0 });
        scene11->Translate(0.0f, 1.0f, 0.0f);
        scene11->Scale(0.5f, 0.5f, 0.5f);

        scene2->SetDraw(mesh, material2, DrawIndexed { 36, 1, 0, 0, 0 });
        scene2->Translate(-1.0f, 0.0f, 0.0f);

        scene21->SetDraw(mesh, material2, DrawIndexed { 36, 1, 0, 0, 0 });
        scene21->Translate(0.0f, 1.0f, 0.0f);
        scene21->Scale(0.5f, 0.5f, 0.5f);
    }

    // render
    while (!window->ShouldClose()) {
        // query image from swapchain
        auto frame = window->AcquireNext();

        // create a camera
        auto camera = SlimPtr<Camera>("camera");
        camera->LookAt(glm::vec3(0.0, 0.0, 3.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
        camera->Perspective(1.05, frame->GetAspectRatio(), 0.1, 20.0f);

        // transform scene nodes
        scene1->Rotate(glm::vec3(0.0f, 1.0f, 0.0f), +0.025);
        scene2->Rotate(glm::vec3(0.0f, 1.0f, 0.0f), -0.025);
        scene->Update();

        // sceneFilter result + sorting
        auto culling = SlimPtr<CPUCulling>();
        culling->Cull(scene, camera);
        culling->Sort(RenderQueue::Geometry,    RenderQueue::GeometryLast, SortingOrder::FrontToback);
        culling->Sort(RenderQueue::Transparent, RenderQueue::Transparent,  SortingOrder::BackToFront);

        // rendergraph-based design
        RenderGraph renderGraph(frame);
        {
            auto colorBuffer = renderGraph.CreateResource(frame->GetBackBuffer());
            auto depthBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_D24_UNORM_S8_UINT, VK_SAMPLE_COUNT_1_BIT);

            auto colorPass = renderGraph.CreateRenderPass("color");
            colorPass->SetColor(colorBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            colorPass->SetDepthStencil(depthBuffer, ClearValue(1.0f, 0));
            colorPass->Execute([&](const RenderInfo &info) {
                MeshRenderer renderer(info);
                renderer.Draw(camera, culling->GetDrawables(RenderQueue::Geometry, RenderQueue::GeometryLast));
            });
        }
        renderGraph.Execute();

        // window update
        window->PollEvents();
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
