#include "viewer.h"

GLTFViewer::GLTFViewer() {
    InitContext();
    InitDevice();
    InitWindow();
    InitInput();
    InitCamera();
    InitGizmo();
    LoadModel();
}

GLTFViewer::~GLTFViewer() {
    device->WaitIdle();
}

void GLTFViewer::Run() {
    while (window->IsRunning()) {
        Window::PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();

        // update camera projection
        camera->Perspective(1.05, frame->GetAspectRatio(), 0.1, 2000.0f);

        // update input for arcball
        arcball.SetExtent(frame->GetExtent());
        arcball.Update(input);

        CPUCulling gizmoFilter;
        CPUCulling sceneFilter;

        // update scene nodes
        if (!model.scenes.empty()) {
            GLTFScene& scene = model.scenes[0];
            for (auto node : scene.roots) {
                node->SetTransform(arcball.GetTransform());
                node->Update();
                sceneFilter.Cull(node, camera);
            }
            sceneFilter.Sort(RenderQueue::Geometry,    RenderQueue::GeometryLast, SortingOrder::FrontToback);
            sceneFilter.Sort(RenderQueue::Transparent, RenderQueue::Transparent,  SortingOrder::BackToFront);
        }

        // add gizmo
        gizmo->scene->SetTransform(arcball.GetTransformNoScale());
        gizmo->scene->Scale(0.1, 0.1, 0.1);
        gizmo->scene->Update();
        gizmoFilter.Cull(gizmo->scene, camera);
        gizmoFilter.Sort(RenderQueue::Geometry,    RenderQueue::GeometryLast, SortingOrder::FrontToback);
        gizmoFilter.Sort(RenderQueue::Transparent, RenderQueue::Transparent,  SortingOrder::BackToFront);

        // rendergraph-based design
        RenderGraph renderGraph(frame);
        {
            auto backBuffer = renderGraph.CreateResource(frame->GetBackBuffer());
            auto depthBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_D24_UNORM_S8_UINT, msaa);
            auto gizmoDepthBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_D24_UNORM_S8_UINT, VK_SAMPLE_COUNT_1_BIT);
            auto colorBuffer = msaa > 1
                             ? renderGraph.CreateResource(frame->GetExtent(), backBuffer->GetImage()->GetFormat(), msaa)
                             : backBuffer;

            auto colorPass = renderGraph.CreateRenderPass("color");
            colorPass->SetColor(colorBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            colorPass->SetDepthStencil(depthBuffer, ClearValue(1.0f, 0));
            if (msaa > 1) {
                colorPass->SetColorResolve(backBuffer);
            }
            colorPass->Execute([&](const RenderInfo &info) {
                MeshRenderer renderer(info);
                renderer.Draw(camera, sceneFilter.GetDrawables(RenderQueue::Geometry, RenderQueue::GeometryLast));
            });

            auto gizmoPass = renderGraph.CreateRenderPass("gizmo");
            gizmoPass->SetColor(backBuffer);
            gizmoPass->SetDepthStencil(gizmoDepthBuffer, ClearValue(1.0f, 0));
            gizmoPass->Execute([&](const RenderInfo &info) {
                MeshRenderer renderer(info);
                renderer.Draw(camera, gizmoFilter.GetDrawables(RenderQueue::Geometry, RenderQueue::GeometryLast));
            });
        }
        renderGraph.Execute();

        // reset input
        input->Reset();
    }
}

void GLTFViewer::InitContext() {
    context = SlimPtr<Context>(
        ContextDesc()
            .EnableCompute(true)
            .EnableGraphics(true)
            .EnableValidation(true)
            .EnableGLFW(true)
    );
}

void GLTFViewer::InitDevice() {
    device = SlimPtr<Device>(context);
}

void GLTFViewer::InitWindow() {
    window = SlimPtr<Window>(
        device,
        WindowDesc()
            .SetResolution(640, 480)
            .SetResizable(true)
            .SetTitle("GLTFViewer")
    );
}

void GLTFViewer::InitInput() {
    input = SlimPtr<Input>(window);
    arcball.SetDamping(0.9);
    arcball.SetSensitivity(0.5);
    arcball.SetExtent(window->GetExtent());
}

void GLTFViewer::InitCamera() {
    VkExtent2D extent = window->GetExtent();
    float aspectRatio = static_cast<float>(extent.width) / static_cast<float>(extent.height);

    camera = SlimPtr<Camera>("Camera");
    camera->LookAt(glm::vec3(0.0, 0.0, 3.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
    camera->Perspective(1.05, aspectRatio, 0.1, 2000.0f);
}

void GLTFViewer::InitGizmo() {
    device->Execute([&](CommandBuffer* commandBuffer) {
        gizmo = SlimPtr<Gizmo>(commandBuffer);
        gizmo->scene->Update();
    });
}

void GLTFViewer::LoadModel() {
    manager = SlimPtr<GLTFAssetManager>(device);
    device->Execute([&](CommandBuffer* commandBuffer) {
        model = manager->Load(commandBuffer, ToAssetPath("Objects/DamagedHelmet/glTF/DamagedHelmet.gltf"));
    });
}
