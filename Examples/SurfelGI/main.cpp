#include <slim/slim.hpp>
#include "config.h"
#include "scene.h"
#include "surfel.h"
#include "gbuffer.h"
#include "visualize.h"
#include "composer.h"
#include "overlay.h"

using namespace slim;

int main() {
    slim::Initialize();

    // create a slim device
    auto context = SlimPtr<Context>(
        ContextDesc()
            .Verbose(true)
            .EnableCompute(true)
            .EnableGraphics(true)
            .EnableValidation(true)
            .EnableGLFW(true)
            .EnableDescriptorIndexing()
            .EnableMultiDraw()
            .EnableShaderInt64()
            #ifdef ENABLE_RAY_TRACING
            .EnableBufferDeviceAddress()
            .EnableRayTracing()
            #endif
    );

    // create a slim device
    auto device = SlimPtr<Device>(context);

    // create a slim window
    auto window = SlimPtr<Window>(
        device,
        WindowDesc()
            .SetResolution(640, 480)
            .SetResizable(true)
            .SetTitle("SurfelGI")
            .EnableFPS(true)
    );

    // ui and input control
    auto ui = SlimPtr<DearImGui>(device, window);
    auto input = SlimPtr<Input>(window);
    auto time = SlimPtr<Time>();

    // scene
    auto scene = MainScene(device);

    // surfel
    auto surfel = SurfelManager(device);

    // resource pool
    AutoReleasePool pool(device);

    int frameId = 0;

    // render
    while (window->IsRunning()) {
        Window::PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();

        // configure camera
        scene.camera->Perspective(1.05, frame->GetAspectRatio(), scene.GetNear(), scene.GetFar());
        scene.camera->SetExtent(frame->GetExtent());
        scene.camera->Update(input, time);

        // update time
        time->Update();

        // rendergraph-based design
        RenderGraph renderGraph(frame);
        {
            VkExtent2D frameExtent = frame->GetExtent();

            VkExtent2D barExtent = frameExtent;
            barExtent.height = 20;

            auto colorBuffer       = renderGraph.CreateResource(frame->GetBackBuffer());

            // scene resources
            scene.lightResource   = renderGraph.CreateResource(scene.lightBuffer);
            scene.cameraResource   = renderGraph.CreateResource(scene.cameraBuffer);

            // surfel resources
            surfel.surfelCovBuffer = renderGraph.CreateResource(frameExtent, VK_FORMAT_R32_SFLOAT,          VK_SAMPLE_COUNT_1_BIT);

            // gbuffer resources
            GBuffer gbuffer = {};
            gbuffer.albedoBuffer   = renderGraph.CreateResource(frameExtent, VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT);
            gbuffer.normalBuffer   = renderGraph.CreateResource(frameExtent, VK_FORMAT_R8G8B8A8_SNORM,      VK_SAMPLE_COUNT_1_BIT);
            gbuffer.positionBuffer = renderGraph.CreateResource(frameExtent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT);
            gbuffer.objectBuffer   = renderGraph.CreateResource(frameExtent, VK_FORMAT_R32_UINT,            VK_SAMPLE_COUNT_1_BIT);
            gbuffer.depthBuffer    = renderGraph.CreateResource(frameExtent, VK_FORMAT_D32_SFLOAT,          VK_SAMPLE_COUNT_1_BIT);

            // debug resources
            Visualize vis = {};
            vis.objectBuffer       = renderGraph.CreateResource(frameExtent, VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT);
            vis.depthBuffer        = renderGraph.CreateResource(frameExtent, VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT);
            vis.surfelCovBuffer    = renderGraph.CreateResource(frameExtent, VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT);
            vis.surfelGridBuffer   = renderGraph.CreateResource(frameExtent, VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT);
            vis.surfelAllocBuffer  = renderGraph.CreateResource(barExtent,   VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT);

            // prepare
            AddScenePreparePass(renderGraph, &scene);

            // draw gbuffer
            AddGBufferPass(renderGraph, pool, &gbuffer, &scene);

            // spawn surfels based on iterative hole filling algorithm
            AddSurfelPass(renderGraph, pool, &scene, &gbuffer, &vis, &surfel, frameId);

            // compose
            AddComposerPass(renderGraph, pool, colorBuffer, &gbuffer, &scene);

            // debug and ui passes
            if (1) {
                // visualize object ID
                AddObjectVisPass(renderGraph, pool, vis.objectBuffer, gbuffer.objectBuffer);

                // visualize linear depth
                AddLinearDepthVisPass(renderGraph, pool, &scene, vis.depthBuffer, gbuffer.depthBuffer);

                // surfel grid vis
                AddSurfelGridVisPass(renderGraph, pool, &scene, vis.surfelGridBuffer, gbuffer.depthBuffer);

                // surfel alloc vis
                AddSurfelAllocVisPass(renderGraph, pool, surfel.surfelStatBuffer, vis.surfelAllocBuffer);

                // overlay
                AddOverlayPass(renderGraph, colorBuffer, &gbuffer, &vis, &surfel, ui);
            }
        }
        renderGraph.Execute();

        #ifdef ENABLE_RAY_TRACING
        // update surfel aabbs
        surfel.UpdateAABB();
        #endif

        input->Reset();
        frameId++;
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
