#include <slim/slim.hpp>
#include "config.h"
#include "scene.h"
#include "gbuffer.h"
#include "raytrace.h"
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
            #ifdef ENABLE_RAY_TRACING
            .EnableBufferDeviceAddress()
            .EnableShaderInt64()
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
            .SetTitle("Hybrid")
            .EnableFPS(true)
    );

    // ui and input control
    auto ui = SlimPtr<DearImGui>(device, window);
    auto input = SlimPtr<Input>(window);
    auto time = SlimPtr<Time>();

    // camera
    auto camera = SlimPtr<Flycam>("camera");
    camera->SetWalkSpeed(20.0f);
    camera->LookAt(glm::vec3(3.0, 135.0, 0.0), glm::vec3(0.0, 135.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

    // scene
    auto scene = MainScene(device);

    // light
    auto dirLight = DirectionalLight {
        glm::vec4(0.0, -1.0, 0.0, 0.0), // direction
        glm::vec4(1.0, 1.0, 1.0, 1.0),  // light color
    };

    // resource bundle
    ResourceBundle bundle;

    // render
    while (window->IsRunning()) {
        Window::PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();

        // configure camera
        camera->Perspective(1.05, frame->GetAspectRatio(), 0.1, 2000.0f);
        camera->SetExtent(frame->GetExtent());
        camera->Update(input, time);

        // update time
        time->Update();

        // rendergraph-based design
        RenderGraph renderGraph(frame);
        {
            auto colorBuffer = renderGraph.CreateResource(frame->GetBackBuffer());

            // resources
            GBuffer gbuffer = {};
            gbuffer.albedoBuffer   = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT);
            gbuffer.normalBuffer   = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_SNORM,      VK_SAMPLE_COUNT_1_BIT);
            gbuffer.positionBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT);
            gbuffer.objectBuffer   = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R32_UINT,            VK_SAMPLE_COUNT_1_BIT);
            gbuffer.depthBuffer    = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_D24_UNORM_S8_UINT,   VK_SAMPLE_COUNT_1_BIT);

            // draw gbuffer
            AddGBufferPass(renderGraph, bundle, camera, &gbuffer, &scene);

            #ifdef ENABLE_RAY_TRACING
            // hybrid ray tracer / rasterizer
            AddRayTracePass(renderGraph, bundle, &gbuffer, scene.GetTlas(), &dirLight);
            #endif

            // compose
            AddComposerPass(renderGraph, bundle, colorBuffer, &gbuffer, &dirLight);

            // // overlay
            // AddOverlayPass(renderGraph, bundle, colorBuffer, &gbuffer, ui);
        }
        renderGraph.Execute();
        input->Reset();
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
