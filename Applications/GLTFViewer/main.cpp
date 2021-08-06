#include <slim/slim.hpp>

#include "model.h"

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
    auto device = SlimPtr<Device>(context);

    // create a slim window
    auto window = SlimPtr<Window>(
        device,
        WindowDesc()
            .SetResolution(640, 480)
            .SetResizable(true)
            .SetTitle("GLTFViewer")
    );

    GLTFModel model;
    GLTFAssetManager manager(device);

    device->Execute([&](CommandBuffer* commandBuffer) {
        model = manager.Load(commandBuffer, ToAssetPath("Characters/GenshinImpact/amber/scene.gltf"));
        // model = manager.Load(commandBuffer, ToAssetPath("Scenes/Sponza/glTF/Sponza.gltf"));
    });

    GLTFScene& scene = model.scenes[0];
    scene.roots[0]->Scale(0.1, 0.1, 0.1);
    scene.roots[0]->Rotate(glm::vec3(0.0f, 0.0f, 1.0f),  M_PI / 2.0f);
    scene.roots[0]->Rotate(glm::vec3(1.0f, 0.0f, 0.0f), -M_PI / 2.0f);
    scene.roots[0]->Rotate(glm::vec3(0.0f, 0.0f, 1.0f),  M_PI       );
    scene.roots[0]->Translate(0, 0, -10);

    // render
    while (!window->ShouldClose()) {
        // query image from swapchain
        auto frame = window->AcquireNext();

        // create a camera
        auto camera = SlimPtr<Camera>("camera");
        camera->LookAt(glm::vec3(0.0, 0.0, 3.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
        camera->Perspective(1.05, frame->GetAspectRatio(), 0.1, 20.0f);

        // transform scene nodes
        for (auto root : scene.roots) {
            root->Update();
        }

        // perform culling
        SceneFilter filter;
        for (auto root : scene.roots) {
            root->Update();
            filter.Cull(root, camera);
        }

        // perform sorting
        filter.Sort(RenderQueue::Geometry,    RenderQueue::GeometryLast, SortingOrder::FrontToback);
        filter.Sort(RenderQueue::Transparent, RenderQueue::Transparent,  SortingOrder::BackToFront);

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
                renderer.Draw(camera, filter.GetDrawables(RenderQueue::Geometry, RenderQueue::GeometryLast));
            });
        }
        renderGraph.Execute();

        // window update
        window->PollEvents();
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
