#include "viewer.h"

GLTFViewer::GLTFViewer() {
    InitContext();
    InitWindow();
    InitInput();
    InitCamera();
    InitGizmo();
    InitLUT();
    InitSampler();
    InitSkybox();
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
        arcball->Perspective(1.05, frame->GetAspectRatio(), 0.1, 2000.0f);

        // update input for arcball
        arcball->SetExtent(frame->GetExtent());
        arcball->Update(input);

        CPUCulling sceneFilter;
        CPUCulling gizmoFilter;

        // update scene nodes
        sceneFilter.Cull(skybox->scene, arcball);
        if (root) {
            root->SetTransform(arcball->GetModelMatrix());
            root->Update();
            sceneFilter.Cull(root, arcball);
        }
        sceneFilter.Sort(RenderQueue::Geometry,    RenderQueue::GeometryLast, SortingOrder::FrontToback);
        sceneFilter.Sort(RenderQueue::Transparent, RenderQueue::Transparent,  SortingOrder::BackToFront);

        // add gizmo
        gizmo->scene->SetTransform(arcball->GetModelMatrix(false));
        gizmo->scene->Scale(0.1, 0.1, 0.1);
        gizmo->scene->Update();
        gizmoFilter.Cull(gizmo->scene, arcball);
        gizmoFilter.Sort(RenderQueue::Geometry,    RenderQueue::GeometryLast, SortingOrder::FrontToback);
        gizmoFilter.Sort(RenderQueue::Transparent, RenderQueue::Transparent,  SortingOrder::BackToFront);

        // rendergraph-based design
        RenderGraph renderGraph(frame);
        {
            auto backBuffer = renderGraph.CreateResource(frame->GetBackBuffer());
            auto depthBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_D24_UNORM_S8_UINT, msaa);
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
                renderer.Draw(arcball, sceneFilter.GetDrawables(RenderQueue::Geometry, RenderQueue::GeometryLast));
            });

            #if 1
            auto gizmoDepthBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_D24_UNORM_S8_UINT, VK_SAMPLE_COUNT_1_BIT);
            auto gizmoPass = renderGraph.CreateRenderPass("gizmo");
            gizmoPass->SetColor(backBuffer);
            gizmoPass->SetDepthStencil(gizmoDepthBuffer, ClearValue(1.0f, 0));
            gizmoPass->Execute([&](const RenderInfo &info) {
                MeshRenderer renderer(info);
                renderer.Draw(arcball, gizmoFilter.GetDrawables(RenderQueue::Geometry, RenderQueue::GeometryLast));
            });
            #endif
        }
        renderGraph.Execute();

        // reset input
        input->Reset();
    }
}

void GLTFViewer::InitContext() {
    // ContextDesc object needs to be alive when creating context ande device.
    // This is something needs to be fixed.
    auto contextDesc = ContextDesc()
            .EnableCompute(true)
            .EnableGraphics(true)
            .EnableValidation(true)
            .EnableGLFW(true)
            .EnableDescriptorIndexing();

    context = SlimPtr<Context>(contextDesc);
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
}

void GLTFViewer::InitCamera() {
    arcball = SlimPtr<Arcball>();
    arcball->LookAt(glm::vec3(0.0, 0.0, 3.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
    arcball->SetDamping(0.95);
    arcball->SetSensitivity(0.5);
    arcball->SetExtent(window->GetExtent());
}

void GLTFViewer::InitGizmo() {
    device->Execute([&](CommandBuffer* commandBuffer) {
        gizmo = SlimPtr<Gizmo>(commandBuffer);
        gizmo->scene->Update();
    });
}

void GLTFViewer::InitSkybox() {
    device->Execute([&](CommandBuffer* commandBuffer) {
        skybox = SlimPtr<Skybox>(commandBuffer);
        gizmo->scene->Update();
    });
}

void GLTFViewer::InitLUT() {
    device->Execute([&](CommandBuffer* commandBuffer) {
        dfglut = TextureLoader::Load2D(commandBuffer, ToAssetPath("Pictures/ibl_brdf_lut.png"));
    });
}

void GLTFViewer::InitSampler() {
    dfglutSampler = SlimPtr<Sampler>(
        device,
        SamplerDesc()
            .AddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
    );

    diffuseSampler = SlimPtr<Sampler>(
        device,
        SamplerDesc()
    );

    specularSampler = SlimPtr<Sampler>(
        device,
        SamplerDesc()
            .LOD(0.0, 12.0)
    );
}

void GLTFViewer::LoadModel() {
    manager = SlimPtr<GLTFAssetManager>(device);
    device->Execute([&](CommandBuffer* commandBuffer) {
        model = manager->Load(commandBuffer, ToAssetPath("Objects/DamagedHelmet/glTF/DamagedHelmet.gltf"));
        // model = manager->Load(commandBuffer, "/Users/tcheng/Downloads/glTF-Sample-Models/2.0/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf");
        // model = manager->Load(commandBuffer, "/Users/tcheng/Downloads/glTF-Sample-Models/2.0/WaterBottle/glTF/WaterBottle.gltf");

        // attaching lut + env to materials
        for (auto material: model.materials) {
            material->SetTexture("DFGLUT", dfglut, dfglutSampler);
            material->SetTexture("EnvironmentTexture", skybox->skybox, specularSampler);
            material->SetTexture("IrradianceTexture", skybox->irradiance, diffuseSampler);
        }

        // adding a wrapper node for transform control
        scene = SlimPtr<SceneManager>();
        root = scene->Create<Scene>("root");

        GLTFScene& scene = model.scenes[0];
        for (auto node : scene.roots) {
            root->AddChild(node);
        }
        root->Update();
    });
}
