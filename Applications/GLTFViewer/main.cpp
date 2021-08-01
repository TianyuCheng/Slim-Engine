#include <slim/slim.hpp>

#include "model.h"
#include "material.h"

using namespace slim;

int main() {
    // // create a slim device
    // auto context = SlimPtr<Context>(
    //     ContextDesc()
    //         .EnableCompute(true)
    //         .EnableGraphics(true)
    //         .EnableValidation(true)
    //         .EnableGLFW(true)
    // );
    //
    // // create a slim device
    // auto device = SlimPtr<Device>(context);
    //
    // // create a slim window
    // auto window = SlimPtr<Window>(
    //     device,
    //     WindowDesc()
    //         .SetResolution(640, 480)
    //         .SetResizable(true)
    //         .SetTitle("Depth Buffering")
    // );
    //
    // // load gltf model
    // GLTFModel model;
    // LoadModel(context, model, ToAssetPath("Characters/GenshinImpact/amber/scene.gltf"));
    // // LoadModel(context, model, ToAssetPath("glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf"));
    //
    // context->WaitIdle();
    //
    // // render
    // auto window = context->GetWindow();
    // while (!window->ShouldClose()) {
    //     // query image from swapchain
    //     auto frame = window->AcquireNext();
    //
    //     // create a camera
    //     Camera camera("camera");
    //     camera.LookAt(glm::vec3(3.0, 3.0, 20.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
    //     camera.Perspective(1.05, frame->GetAspectRatio(), 0.1, 50.0f);
    //
    //     for (SceneNode* root : *model.defaultScene) {
    //         root->Rotate(glm::vec3(0.0, 0.0, 1.0), 0.01);
    //     }
    //
    //     model.defaultScene->Update();
    //     model.defaultScene->Cull(camera);
    //
    //     // rendergraph-based design
    //     RenderGraph graph(frame);
    //     {
    //         auto colorBuffer = graph.CreateResource(frame->GetBackBuffer());
    //         auto depthBuffer = graph.CreateResource(frame->GetExtent(), VK_FORMAT_D24_UNORM_S8_UINT, VK_SAMPLE_COUNT_1_BIT);
    //
    //         auto colorPass = graph.CreateRenderPass("color");
    //         colorPass->SetColor(colorBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    //         colorPass->SetDepthStencil(depthBuffer, ClearValue(1.0f, 0));
    //         colorPass->Execute([=](const RenderInfo &info) {
    //             model.defaultScene->Render(graph, camera, RenderQueue::Opaque, SortingOrder::FrontToback);
    //         });
    //     }
    //
    //     graph.Execute();
    //
    //     // window update
    //     window->PollEvents();
    // }
    //
    // context->WaitIdle();
    // return EXIT_SUCCESS;
}
