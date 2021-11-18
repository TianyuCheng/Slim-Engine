#include <slim/slim.hpp>
using namespace slim;

#include "config.h"
#include "scene.h"
#include "render.h"
#include "update.h"
#include "debug.h"
#include "surfel.h"
#include "direct.h"
#include "gbuffer.h"
#include "compose.h"
#include "overlay.h"

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
            .EnableNonSolidPolygonMode()
            .EnableDescriptorIndexing()
            .EnableBufferDeviceAddress()
            .EnableShaderInt64()
            .EnableRayTracing()
    );

    // create a slim device
    auto device = SlimPtr<Device>(context);

    // create a slim window
    auto window = SlimPtr<Window>(
        device,
        WindowDesc()
            .SetResolution(960, 720)
            .SetResizable(true)
            .SetTitle("Surfel GI")
            .EnableFPS(true)
    );

    // create ui handle
    auto ui = SlimPtr<DearImGui>(device, window, []() {
        DearImGui::EnableDocking();
        DearImGui::EnableMultiview();
    });

    // input control
    auto input = SlimPtr<Input>(window);
    auto time = SlimPtr<Time>();

    // resources
    auto pool = AutoReleasePool(device);

    // scene
    auto scene = Scene(device);
    if (1) {
        LightInfo light = {};
        light.type = LIGHT_TYPE_POINT;
        light.color = vec3(1.0, 1.0, 1.0);
        light.intensity = 1.0;
        light.range = 5.0;
        light.position = vec3(-2.0, 1.0, 1.0);
        scene.lights.push_back(light);
    }
    if (1) {
        scene.sky.color = vec3(0.0);
    }

    // build ui dockspace
    BuildOverlayUI(ui);

    // render
    while (window->IsRunning()) {
        Window::PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();

        // configure camera
        scene.camera->Perspective(1.05, frame->GetAspectRatio(), scene.Near(), scene.Far());
        scene.camera->SetExtent(frame->GetExtent());
        scene.camera->Update(input, time);

        // bar size
        VkExtent2D barExtent = {};
        barExtent.width = 320;
        barExtent.height = 2;

        // update time
        time->Update();

        // rendergraph-based design
        RenderGraph graph(frame);
        {
            auto backBuffer             = graph.CreateResource(frame->GetBackBuffer());

            render::SceneData sceneData = {};
            sceneData.camera            = graph.CreateResource(scene.cameraBuffer);
            sceneData.sky               = graph.CreateResource(scene.skyBuffer);
            sceneData.lights            = graph.CreateResource(scene.lightBuffer);
            sceneData.frame             = graph.CreateResource(scene.frameInfoBuffer);
            sceneData.lightXform        = graph.CreateResource(scene.lightXformBuffer);

            // gbuffer resources
            render::GBuffer gbuffer     = {};
            gbuffer.albedo              = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);
            gbuffer.emissive            = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);
            gbuffer.metallicRoughness   = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8_UNORM,     VK_SAMPLE_COUNT_1_BIT);
            gbuffer.normal              = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_SNORM, VK_SAMPLE_COUNT_1_BIT);
            gbuffer.depth               = graph.CreateResource(frame->GetExtent(), VK_FORMAT_D32_SFLOAT,     VK_SAMPLE_COUNT_1_BIT);
            gbuffer.object              = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R32_UINT,       VK_SAMPLE_COUNT_1_BIT);
            gbuffer.specular            = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT);
            gbuffer.globalDiffuse       = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT);
            #ifdef ENABLE_DIRECT_ILLUMINATION
            gbuffer.directDiffuse       = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT);
            #endif
            #ifdef ENABLE_GBUFFER_WORLD_POSITION
            gbuffer.position            = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT);   // debug
            #endif

            // surfel resources
            render::Surfel surfel       = {};
            surfel.surfels              = graph.CreateResource(scene.surfelBuffer);
            surfel.surfelLive           = graph.CreateResource(scene.surfelLiveBuffer);
            surfel.surfelData           = graph.CreateResource(scene.surfelDataBuffer);
            surfel.surfelGrid           = graph.CreateResource(scene.surfelGridBuffer);
            surfel.surfelCell           = graph.CreateResource(scene.surfelCellBuffer);
            surfel.surfelStat           = graph.CreateResource(scene.surfelStatBuffer);
            surfel.surfelDepth          = graph.CreateResource(scene.surfelDepthImage);
            surfel.surfelRayGuide       = graph.CreateResource(scene.surfelRayGuideImage);
            surfel.surfelCoverage       = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R32_SFLOAT, VK_SAMPLE_COUNT_1_BIT);

            // visualize resources
            render::Debug debug         = {};
            debug.depth                 = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R32_SFLOAT,     VK_SAMPLE_COUNT_1_BIT);
            debug.object                = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);
            debug.surfelGrid            = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);
            debug.surfelDebug           = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);
            debug.surfelBudget          = graph.CreateResource(barExtent,          VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);
            debug.surfelVariance        = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);

            AddUpdatePass(graph, pool, &gbuffer, &sceneData, &surfel, &debug, &scene);
            AddGBufferPass(graph, pool, &gbuffer, &sceneData, &surfel, &debug, &scene);
            AddSurfelPass(graph, pool, &gbuffer, &sceneData, &surfel, &debug, &scene);
            #ifdef ENABLE_DIRECT_ILLUMINATION
            AddDirectLightingPass(graph, pool, &gbuffer, &sceneData, &surfel, &debug, &scene);
            #endif
            AddComposePass(graph, pool, &gbuffer, &sceneData, &surfel, &debug, &scene, backBuffer);

            // show light control
            AddLightVisPass(graph, pool, &gbuffer, &sceneData, &surfel, &debug, &scene, backBuffer);

            #ifdef ENABLE_GBUFFER_VISUALIZATION
            AddLinearDepthPass(graph, pool, &gbuffer, &sceneData, &surfel, &debug, &scene);
            AddObjectVisPass(graph, pool, &gbuffer, &sceneData, &surfel, &debug, &scene);
            #endif

            #ifdef ENABLE_SURFEL_GRID_VISUALIZATION
            AddGridVisPass(graph, pool, &gbuffer, &sceneData, &surfel, &debug, &scene);
            #endif

            #ifdef ENABLE_SURFEL_BUDGET_VISUALIZATION
            AddSurfelAllocVisPass(graph, pool, &gbuffer, &sceneData, &surfel, &debug, &scene);
            #endif

            // ui pass
            AddOverlayPass(graph, pool, &gbuffer, &sceneData, &surfel, &debug, &scene, ui, backBuffer);
        }
        graph.Execute();

        // input reset
        input->Reset();
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
