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
            .SetTitle("Depth Buffering")
    );

    // create vertex and fragment shaders
    auto vShader = SlimPtr<spirv::VertexShader>(context, "main", "shaders/simple.vert.spv");
    auto fShader = SlimPtr<spirv::FragmentShader>(context, "main", "shaders/simple.frag.spv");

    // create material and configure material's pipeline
    auto material1 = SlimPtr<Material>(CreateMaterial(vShader, fShader));
    auto material2 = SlimPtr<Material>(CreateMaterial(vShader, fShader));

    // The first material uses a yellow color
    material1->PrepareMaterial = [=](const RenderInfo &info) {
        auto color = glm::vec3(1.0, 1.0, 0.0);
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame, info.material->GetPipeline());
        descriptor->SetUniform("Color", info.renderFrame->RequestUniformBuffer(color));
        info.commandBuffer->BindDescriptor(descriptor);
    };

    // The second material uses a cyan color
    material2->PrepareMaterial = [=](const RenderInfo &info) {
        auto color = glm::vec3(0.0, 1.0, 1.0);
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame, info.material->GetPipeline());
        descriptor->SetUniform("Color", info.renderFrame->RequestUniformBuffer(color));
        info.commandBuffer->BindDescriptor(descriptor);
    };

    // create mesh and its submeshes
    auto mesh = SlimPtr<Mesh>(CreateMesh(context));
    auto submesh = Submesh(mesh, 0, 0, 36);

    // create a scene
    auto scene = SlimPtr<Scene>("scene");
    auto child1 = scene->AddChild("child1");
    auto child11 = child1->AddChild("child1.1");
    auto child2 = scene->AddChild("child2");
    auto child21 = child2->AddChild("child2.1");
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

        child1->Rotate(glm::vec3(0.0f, 1.0f, 0.0f), +0.025);
        child2->Rotate(glm::vec3(0.0f, 1.0f, 0.0f), -0.025);

        // update scene
        scene->Update();

        // rendergraph-based design
        RenderGraph graph(frame);
        {
            auto colorBuffer = graph.CreateResource(frame->GetBackBuffer());
            auto depthBuffer = graph.CreateResource(frame->GetExtent(), VK_FORMAT_D24_UNORM_S8_UINT, VK_SAMPLE_COUNT_1_BIT);

            auto colorPass = graph.CreateRenderPass("color");
            colorPass->SetColor(colorBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            colorPass->SetDepthStencil(depthBuffer, ClearValue(1.0f, 0));
            colorPass->Execute([=](const RenderGraph &graph) {
                scene->Render(graph, camera);
            });
        }

        graph.Execute();

        // window update
        window->PollEvents();
    }

    context->WaitIdle();
    return EXIT_SUCCESS;
}
