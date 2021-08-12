#include <slim/slim.hpp>

#include "config.h"
#include "model.h"
#include "gizmo.h"

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

    // input & controller
    auto input = SlimPtr<Input>(window);
    auto arcball = SlimPtr<Arcball>();
    arcball->SetDamping(0.9);
    arcball->SetSensitivity(0.5);

    GLTFModel model;
    GLTFAssetManager manager(device);

    Gizmo gizmo;

    device->Execute([&](CommandBuffer* commandBuffer) {
        gizmo.Init(commandBuffer);
        gizmo.scene->Update();

        model = manager.Load(commandBuffer, ToAssetPath("Objects/DamagedHelmet/glTF/DamagedHelmet.gltf"));
    });

    GLTFScene& scene = model.scenes[0];

    auto time = Time();

    auto camera = SlimPtr<Camera>("camera");
    camera->LookAt(glm::vec3(0.0, 0.0, 3.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

    // render
    while (!window->ShouldClose()) {
        // query image from swapchain
        auto frame = window->AcquireNext();

        // camera
        camera->Perspective(1.05, frame->GetAspectRatio(), 0.1, 2000.0f);

        // handle input
        arcball->SetExtent(frame->GetExtent());
        arcball->Update(input);

        // transform scene nodes
        for (auto root : scene.roots) {
            root->SetTransform(arcball->GetTransform());
            root->Update();
        }

        gizmo.scene->SetTransform(arcball->GetTransform());
        gizmo.scene->Update();

        // perform culling
        CPUCulling culling;
        for (auto root : scene.roots) {
            root->Update();
            culling.Cull(root, camera);
        }

        // gizmo
        CPUCulling gizmoCulling;
        gizmoCulling.Cull(gizmo.scene, camera);

        // perform sorting
        culling.Sort(RenderQueue::Geometry,    RenderQueue::GeometryLast, SortingOrder::FrontToback);
        culling.Sort(RenderQueue::Transparent, RenderQueue::Transparent,  SortingOrder::BackToFront);

        // rendergraph-based design
        RenderGraph renderGraph(frame);
        {
            auto backBuffer = renderGraph.CreateResource(frame->GetBackBuffer());
            auto depthBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_D24_UNORM_S8_UINT, msaa);

            RenderGraph::Resource* colorBuffer = nullptr;
            if (msaa > 1) {
                colorBuffer = renderGraph.CreateResource(frame->GetExtent(), backBuffer->GetImage()->GetFormat(), msaa);
            } else {
                colorBuffer = backBuffer;
            }

            auto colorPass = renderGraph.CreateRenderPass("color");
            colorPass->SetColor(colorBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            colorPass->SetDepthStencil(depthBuffer, ClearValue(1.0f, 0));
            if (msaa > 1) {
                colorPass->SetColorResolve(backBuffer);
            }
            colorPass->Execute([&](const RenderInfo &info) {
                MeshRenderer renderer(info);
                renderer.Draw(camera, culling.GetDrawables(RenderQueue::Geometry, RenderQueue::GeometryLast));
                renderer.Draw(camera, gizmoCulling.GetDrawables(RenderQueue::Geometry, RenderQueue::GeometryLast));
            });
        }
        renderGraph.Execute();

        // window update
        input->Reset();
        window->PollEvents();
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
