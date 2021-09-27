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

    // camera
    auto camera = SlimPtr<Flycam>("camera");
    camera->SetWalkSpeed(100.0f);
    camera->LookAt(glm::vec3(3.0, 135.0, 0.0), glm::vec3(0.0, 135.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

    // scene
    auto scene = MainScene(device);

    // light
    auto dirLight = DirectionalLight {
        glm::vec4(0.0, -1.0, 0.0, 0.0), // direction
        glm::vec4(1.0, 1.0, 1.0, 1.0),  // light color
    };

    // surfel
    auto surfel = SurfelManager(device, MAX_NUM_SURFELS);

    // resource bundle
    ResourceBundle bundle;

    // render
    while (window->IsRunning()) {
        Window::PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();

        // configure camera
        camera->Perspective(1.05, frame->GetAspectRatio(), 0.1, 3000.0f);
        camera->SetExtent(frame->GetExtent());
        camera->Update(input, time);

        // update time
        time->Update();

        // rendergraph-based design
        RenderGraph renderGraph(frame);
        {
            auto colorBuffer       = renderGraph.CreateResource(frame->GetBackBuffer());

            // gbuffer resources
            GBuffer gbuffer = {};
            gbuffer.albedoBuffer   = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT);
            gbuffer.normalBuffer   = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_SNORM,      VK_SAMPLE_COUNT_1_BIT);
            gbuffer.positionBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT);
            gbuffer.objectBuffer   = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R32_UINT,            VK_SAMPLE_COUNT_1_BIT);
            gbuffer.depthBuffer    = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_D32_SFLOAT,          VK_SAMPLE_COUNT_1_BIT);

            // debug resources
            Visualize vis;
            vis.objectBuffer       = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT);
            vis.depthBuffer        = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT);
            vis.surfelcovBuffer    = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT);

            // draw gbuffer
            AddGBufferPass(renderGraph, bundle, camera, &gbuffer, &scene);

            // spawn surfels based on iterative hole filling algorithm
            AddSurfelCovPass(renderGraph, bundle, camera, &gbuffer, &vis, &surfel);

            // #ifdef ENABLE_RAY_TRACING
            // // hybrid ray tracer / rasterizer
            // AddRayTracePass(renderGraph, bundle, &gbuffer, scene.GetTlas(), &dirLight);
            // #endif

            // compose
            AddComposerPass(renderGraph, bundle, colorBuffer, &gbuffer, &dirLight);

            // [DEBUG] visualize object ID
            AddObjectVisPass(renderGraph, bundle, vis.objectBuffer, gbuffer.objectBuffer);

            // [DEBUG] visualize linear depth
            AddLinearDepthVisPass(renderGraph, bundle, camera, vis.depthBuffer, gbuffer.depthBuffer);

            // [DEBUG] overlay
            AddOverlayPass(renderGraph, bundle, colorBuffer, &gbuffer, &vis, ui);
        }
        renderGraph.Execute();

        input->Reset();
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
