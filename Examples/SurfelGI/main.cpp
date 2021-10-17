#include <slim/slim.hpp>
using namespace slim;

#include "config.h"
#include "scene.h"
#include "render.h"
#include "update.h"
#include "debug.h"
#include "gbuffer.h"
#include "overlay.h"

int3 compute_surfel_grid(const CameraInfo &cam, const vec3& worldPos) {
    vec3 grid = (worldPos / SURFEL_GRID_SIZE) - floor(cam.position / SURFEL_GRID_SIZE);

    int3 innerGrid = int3(floor(grid));
    int3 innerBound = int3(SURFEL_GRID_INNER_DIMS.x / 2,
                           SURFEL_GRID_INNER_DIMS.y / 2,
                           SURFEL_GRID_INNER_DIMS.z / 2);
    // std::cout << "inner bound: " << glm::to_string(innerGrid) << std::endl;
    // std::cout << "inner bound: " << glm::to_string(innerBound) << std::endl;
    glm::bvec3 valid = lessThanEqual(abs(innerGrid), innerBound);
    if (all(valid)) {
        // std::cout << "in linear grids" << std::endl;
        return innerGrid;
    }

    float maxDimValue = std::max(std::max(abs(grid.x), abs(grid.y)), abs(grid.z));

    // if (maxDimValue == abs(grid.z)) {
    //     if (grid.z > 0) {
    //         vec4 tmp0 = cam.surfelGridFrustumPosX * vec4(worldPos, 1.0);
    //         vec3 tmp1 = vec3(tmp0) / tmp0.w;
    //         tmp1.z = tmp1.z * 0.5 + 0.5;
    //         int3 tmp2 = int3(floor(tmp1 * vec3(SURFEL_GRID_OUTER_DIMS)));
    //         // std::cout << "in non-linear grids + Z" << std::endl;
    //         // std::cout << "tmp1: " << glm::to_string(tmp1) << std::endl;
    //         // std::cout << "depth: " << (tmp1.z * 0.5 + 0.5) << std::endl;
    //         // std::cout << "tmp2: " << glm::to_string(tmp2) << std::endl;
    //         return tmp2 + int3(0, 0, SURFEL_GRID_INNER_DIMS.x / 2);
    //     } else {
    //         vec4 tmp0 = cam.surfelGridFrustumNegX * vec4(worldPos, 1.0);
    //         vec3 tmp1 = vec3(tmp0) / tmp0.w;
    //         tmp1.z = tmp1.z * 0.5 + 0.5;
    //         int3 tmp2 = int3(floor(tmp1 * vec3(SURFEL_GRID_OUTER_DIMS)));
    //         // std::cout << "in non-linear grids - Z" << std::endl;
    //         // std::cout << "tmp1: " << glm::to_string(tmp1) << std::endl;
    //         // std::cout << "depth: " << (tmp1.z * 0.5 + 0.5) << std::endl;
    //         // std::cout << "tmp2: " << glm::to_string(tmp2) << std::endl;
    //         return -tmp2 - int3(0, 0, SURFEL_GRID_INNER_DIMS.x / 2);
    //     }
    // }

    if (maxDimValue == abs(grid.z)) {
        if (grid.z > 0) {
            vec4 tmp0 = cam.surfelGridFrustumPosX * vec4(worldPos, 1.0);
            vec3 tmp1 = vec3(tmp0) / tmp0.w;
            // tmp1.z = tmp1.z * 0.5 + 0.5;
            int3 tmp2 = int3(floor(tmp1 * vec3(SURFEL_GRID_OUTER_DIMS)));
            // std::cout << "in non-linear grids + Z" << std::endl;
            // std::cout << "tmp1: " << glm::to_string(tmp1) << std::endl;
            // std::cout << "depth: " << (tmp1.z * 0.5 + 0.5) << std::endl;
            // std::cout << "tmp2: " << glm::to_string(tmp2) << std::endl;
            return tmp2 + int3(0, 0, SURFEL_GRID_INNER_DIMS.x / 2);
        } else {
            vec4 tmp0 = cam.surfelGridFrustumNegX * vec4(worldPos, 1.0);
            vec3 tmp1 = vec3(tmp0) / tmp0.w;
            // tmp1.z = tmp1.z * 0.5 + 0.5;
            int3 tmp2 = int3(floor(tmp1 * vec3(SURFEL_GRID_OUTER_DIMS)));
            // std::cout << "in non-linear grids - Z" << std::endl;
            // std::cout << "tmp1: " << glm::to_string(tmp1) << std::endl;
            // std::cout << "depth: " << (tmp1.z * 0.5 + 0.5) << std::endl;
            // std::cout << "tmp2: " << glm::to_string(tmp2) << std::endl;
            return -tmp2 - int3(0, 0, SURFEL_GRID_INNER_DIMS.x / 2);
        }
    }

    return int3(SURFEL_GRID_DIMS);
}

int main() {

#if 0
    auto camera = Camera("test");
    camera.LookAt(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, 1.0, 0.0));
    camera.Perspective(1.05, 960.0 / 720.0, 0.1, 3000);

    CameraInfo cameraInfo = {};
    cameraInfo.P = camera.GetProjection();
    cameraInfo.V = camera.GetView();
    cameraInfo.invVP = glm::inverse(cameraInfo.P * cameraInfo.V);
    cameraInfo.position = camera.GetPosition();
    cameraInfo.zNear = camera.GetNear();
    cameraInfo.zFar = camera.GetFar();
    cameraInfo.zFarRcp = 1.0f / camera.GetFar();

    glm::mat4 xform = mat4(1.0);
    xform[2][2] = 0.5;
    xform[3][2] = 0.5;

    // transform for non-linear surfel grid transform
    float x = (SURFEL_GRID_INNER_DIMS.x / 2.0);
    float y = (SURFEL_GRID_INNER_DIMS.y / 2.0);
    float z = (SURFEL_GRID_INNER_DIMS.z / 2.0);
    float zFar = cameraInfo.zFar;
    cameraInfo.surfelGridFrustumPosX = xform * glm::frustum(-x, x, -y, y, z, zFar)
                                     * glm::lookAt(camera.GetPosition(),
                                                   camera.GetPosition() + glm::vec3(0.0, 0.0, 1.0),
                                                   glm::vec3(0.0, 1.0, 0.0));
    cameraInfo.surfelGridFrustumNegX = xform * glm::frustum(-x, x, -y, y, z, zFar)
                                     * glm::lookAt(camera.GetPosition(),
                                                   camera.GetPosition() + glm::vec3(0.0, 0.0, -1.0),
                                                   glm::vec3(0.0, 1.0, 0.0));

    for (int i = -10; i <= 10; i++) {
        vec3 worldPos = vec3(0.0, 0.0, 0.0 + 0.5 * i);
        std::cout << "world pos: " << glm::to_string(worldPos) << std::endl;
        std::cout << "grid  pos: " << glm::to_string(compute_surfel_grid(cameraInfo, worldPos)) << std::endl;
        std::cout << "-----------" << std::endl;
    }
#endif

#if 1
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

        // update time
        time->Update();

        // rendergraph-based design
        RenderGraph graph(frame);
        {
            auto backBuffer             = graph.CreateResource(frame->GetBackBuffer());

            render::SceneData sceneData = {};
            sceneData.camera            = graph.CreateResource(scene.cameraBuffer);
            sceneData.lights            = graph.CreateResource(scene.lightBuffer);
            sceneData.frame             = graph.CreateResource(scene.frameInfoBuffer);

            // gbuffer resources
            render::GBuffer gbuffer     = {};
            gbuffer.albedo              = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);
            gbuffer.normal              = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_SNORM, VK_SAMPLE_COUNT_1_BIT);
            gbuffer.depth               = graph.CreateResource(frame->GetExtent(), VK_FORMAT_D32_SFLOAT,     VK_SAMPLE_COUNT_1_BIT);
            gbuffer.object              = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R32_UINT,       VK_SAMPLE_COUNT_1_BIT);

            // surfel resources
            render::Surfel surfel       = {};
            surfel.surfelBuffer         = graph.CreateResource(scene.surfelBuffer);
            surfel.surfelLiveBuffer     = graph.CreateResource(scene.surfelLiveBuffer);
            surfel.surfelFreeBuffer     = graph.CreateResource(scene.surfelFreeBuffer);
            surfel.surfelDataBuffer     = graph.CreateResource(scene.surfelDataBuffer);
            surfel.surfelGridBuffer     = graph.CreateResource(scene.surfelGridBuffer);
            surfel.surfelCellBuffer     = graph.CreateResource(scene.surfelCellBuffer);
            surfel.surfelStatBuffer     = graph.CreateResource(scene.surfelStatBuffer);
            surfel.surfelCoverage       = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R32_SFLOAT,     VK_SAMPLE_COUNT_1_BIT);

            // visualize resources
            render::Debug debug         = {};
            debug.depth                 = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R32_SFLOAT,     VK_SAMPLE_COUNT_1_BIT);
            debug.object                = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);
            debug.surfelGrid            = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);
            debug.surfelDebug           = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);
            debug.surfelRayBudget       = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);
            debug.surfelAllocation      = graph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);

            AddUpdatePass(graph, pool, &gbuffer, &sceneData, &surfel, &debug, &scene);
            AddGBufferPass(graph, pool, &gbuffer, &sceneData, &surfel, &debug, &scene);

            #ifdef ENABLE_GBUFFER_VISUALIZATION
            AddLinearDepthPass(graph, pool, &gbuffer, &sceneData, &surfel, &debug, &scene);
            AddObjectVisPass(graph, pool, &gbuffer, &sceneData, &surfel, &debug, &scene);
            #endif

            #ifdef ENABLE_GRID_VISUALIZATION
            AddGridVisPass(graph, pool, &gbuffer, &sceneData, &surfel, &debug, &scene);
            #endif

            // ui pass
            AddOverlayPass(graph, pool, &gbuffer, &sceneData, &surfel, &debug, &scene, ui, backBuffer);
        }
        graph.Execute();

        // input reset
        input->Reset();
    }

    device->WaitIdle();
#endif
    return EXIT_SUCCESS;
}
