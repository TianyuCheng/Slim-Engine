#include <slim/slim.hpp>
#include "config.h"
#include "scene.h"
#include "surfel.h"
#include "gbuffer.h"
#include "raytrace.h"
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
            .EnableSeparateDepthStencilLayout()
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

    // light
    auto dirLight = DirectionalLight {
        glm::vec4(0.0, -1.0, 0.0, 0.0), // direction
        glm::vec4(1.0, 1.0, 1.0, 1.0),  // light color
    };

    // surfel
    auto surfel = SurfelManager(device, SURFEL_CAPACITY);

    // resource bundle
    ResourceBundle bundle;

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

            // surfel resources
            surfel.surfelCovBuffer = renderGraph.CreateResource(frameExtent, VK_FORMAT_R32_SFLOAT,          VK_SAMPLE_COUNT_1_BIT);

            // gbuffer resources
            GBuffer gbuffer = {};
            gbuffer.albedoBuffer   = renderGraph.CreateResource(frameExtent, VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT);
            gbuffer.normalBuffer   = renderGraph.CreateResource(frameExtent, VK_FORMAT_R8G8B8A8_SNORM,      VK_SAMPLE_COUNT_1_BIT);
            gbuffer.positionBuffer = renderGraph.CreateResource(frameExtent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT);
            gbuffer.objectBuffer   = renderGraph.CreateResource(frameExtent, VK_FORMAT_R32_UINT,            VK_SAMPLE_COUNT_1_BIT);
            gbuffer.depthBuffer    = renderGraph.CreateResource(frameExtent, VK_FORMAT_D32_SFLOAT,          VK_SAMPLE_COUNT_1_BIT);

            // raytrace resources
            RayTrace raytrace = {};
            raytrace.shadowBuffer  = renderGraph.CreateResource(frameExtent, VK_FORMAT_R8_UINT,             VK_SAMPLE_COUNT_1_BIT);
            raytrace.surfelBuffer  = renderGraph.CreateResource(frameExtent, VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT);

            // debug resources
            Visualize vis;
            vis.objectBuffer       = renderGraph.CreateResource(frameExtent, VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT);
            vis.depthBuffer        = renderGraph.CreateResource(frameExtent, VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT);
            vis.surfelCovBuffer    = renderGraph.CreateResource(frameExtent, VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT);
            vis.surfelAllocBuffer  = renderGraph.CreateResource(barExtent,   VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT);

            // draw gbuffer
            AddGBufferPass(renderGraph, bundle, scene.camera, &gbuffer, &scene);

            // #ifdef ENABLE_RAY_TRACING
            // // hybrid ray tracer / rasterizer
            // AddRayTracePass(renderGraph, bundle, &gbuffer, &raytrace, scene.GetTlas(), scene.camera, &dirLight);
            // #endif

            // spawn surfels based on iterative hole filling algorithm
            AddSurfelPass(renderGraph, bundle, scene.camera, &gbuffer, &raytrace, &vis, &surfel);

            // compose
            AddComposerPass(renderGraph, bundle, colorBuffer, &gbuffer, &dirLight);

            // debug and ui passes
            {
                // visualize object ID
                AddObjectVisPass(renderGraph, bundle, vis.objectBuffer, gbuffer.objectBuffer);

                // visualize linear depth
                AddLinearDepthVisPass(renderGraph, bundle, scene.camera, vis.depthBuffer, gbuffer.depthBuffer);

                // surfel alloc vis
                AddSurfelAllocVisPass(renderGraph, bundle, surfel.surfelStatBuffer, vis.surfelAllocBuffer);

                // overlay
                AddOverlayPass(renderGraph, bundle, colorBuffer, &gbuffer, &vis, &surfel, ui);
            }
        }
        // renderGraph.Print();
        renderGraph.Execute();

        input->Reset();

        // if (frameId++ > 2) {
        //     break;
        // }
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
