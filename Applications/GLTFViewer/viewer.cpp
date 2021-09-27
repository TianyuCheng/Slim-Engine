#include "viewer.h"

GLTFViewer::GLTFViewer() {
    InitContext();
    InitDevice();
    InitWindow();
    InitInput();
    InitCamera();
    InitGizmo();
    InitLUT();
    InitPBR();
    InitSampler();
    InitSkybox();
    LoadModel();
    builder->Build();
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
        camera->Perspective(1.05, frame->GetAspectRatio(), 0.1, 2000.0);

        // update input for camera
        camera->SetExtent(window->GetExtent());
        camera->Update(input);
        time->Update();

        CPUCulling skyboxFilter;
        CPUCulling sceneFilter;
        CPUCulling gizmoFilter;

        // update scene nodes
        if (root) {
            sceneFilter.Cull(root, camera);
        }
        sceneFilter.Sort(RenderQueue::Geometry,    RenderQueue::GeometryLast, SortingOrder::FrontToback);
        sceneFilter.Sort(RenderQueue::Transparent, RenderQueue::Transparent,  SortingOrder::BackToFront);

        // update skybox
        skyboxFilter.Cull(skybox->scene, camera);
        skyboxFilter.Sort(RenderQueue::Geometry,    RenderQueue::GeometryLast, SortingOrder::FrontToback);
        skyboxFilter.Sort(RenderQueue::Transparent, RenderQueue::Transparent,  SortingOrder::BackToFront);

        // add gizmo
        gizmoFilter.Cull(gizmo->scene, camera);
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
                renderer.Draw(camera, skyboxFilter.GetDrawables(RenderQueue::Geometry, RenderQueue::GeometryLast));
                renderer.Draw(camera, sceneFilter.GetDrawables(RenderQueue::Geometry, RenderQueue::GeometryLast));
            });

            auto gizmoDepthBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_D24_UNORM_S8_UINT, VK_SAMPLE_COUNT_1_BIT);
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
    // ContextDesc object needs to be alive when creating context ande device.
    // This is something needs to be fixed.
    auto contextDesc = ContextDesc()
            .Verbose(true)
            .EnableCompute(true)
            .EnableGraphics(true)
            .EnableValidation(true)
            .EnableGLFW(true)
            .EnableDescriptorIndexing();

    context = SlimPtr<Context>(contextDesc);
}

void GLTFViewer::InitDevice() {
    device = SlimPtr<Device>(context);
    builder = SlimPtr<scene::Builder>(device);
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
    time = SlimPtr<Time>();
}

void GLTFViewer::InitCamera() {
    camera = SlimPtr<Arcball>();
    camera->LookAt(glm::vec3(0.0, 0.0, 3.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
    camera->SetDamping(0.95);
    camera->SetSensitivity(0.5);
    camera->SetExtent(window->GetExtent());
}

void GLTFViewer::InitGizmo() {
    device->Execute([&](CommandBuffer* commandBuffer) {
        gizmo = SlimPtr<Gizmo>(commandBuffer, builder);
        gizmo->scene->Scale(0.1, 0.1, 0.1);
        gizmo->scene->ApplyTransform();
    });
}

void GLTFViewer::InitSkybox() {
    device->Execute([&](CommandBuffer* commandBuffer) {
        skybox = SlimPtr<Skybox>(commandBuffer, builder);
        skybox->scene->ApplyTransform();
    });
}

void GLTFViewer::InitLUT() {
    device->Execute([&](CommandBuffer* commandBuffer) {
        dfglut = TextureLoader::Load2D(commandBuffer, GetLibraryAsset("BXDF/ibl_brdf_lut.png"));
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

void GLTFViewer::InitPBR() {
    // vertex shader
    vShaderPbr = SlimPtr<spirv::VertexShader>(device, "main", "shaders/gltf.vert.spv");

    // fragment shader
    fShaderPbr = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/gltf.frag.spv");

    auto pipelinePbrDesc =
        GraphicsPipelineDesc()
            .AddVertexBinding(0, sizeof(gltf::Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
                { 0, VK_FORMAT_R32G32B32_SFLOAT,    static_cast<uint32_t>(offsetof(gltf::Vertex, position)) },
                { 1, VK_FORMAT_R32G32B32_SFLOAT,    static_cast<uint32_t>(offsetof(gltf::Vertex, normal  )) },
                { 2, VK_FORMAT_R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(gltf::Vertex, tangent )) },
                { 3, VK_FORMAT_R32G32_SFLOAT,       static_cast<uint32_t>(offsetof(gltf::Vertex, uv0     )) },
                { 4, VK_FORMAT_R32G32_SFLOAT,       static_cast<uint32_t>(offsetof(gltf::Vertex, uv1     )) },
                { 5, VK_FORMAT_R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(gltf::Vertex, color0  )) },
                { 6, VK_FORMAT_R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(gltf::Vertex, joints0 )) },
                { 7, VK_FORMAT_R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(gltf::Vertex, weights0)) },
            })
            .SetVertexShader(vShaderPbr)
            .SetFragmentShader(fShaderPbr)
            .SetCullMode(VK_CULL_MODE_BACK_BIT)
            .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .SetDepthTest(VK_COMPARE_OP_LESS_OR_EQUAL)
            .SetSampleCount(msaa)
            .SetPipelineLayout(PipelineLayoutDesc()
                .AddBinding("Camera",                   SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("MaterialFactors",          SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT)
                .AddBinding("BaseColorTexture",         SetBinding { 1, 1 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT)
                .AddBinding("MetallicRoughnessTexture", SetBinding { 1, 2 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT)
                .AddBinding("NormalTexture",            SetBinding { 1, 3 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT)
                .AddBinding("EmissiveTexture",          SetBinding { 1, 4 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT)
                .AddBinding("OcclusionTexture",         SetBinding { 1, 5 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT)
                .AddBinding("Model",                    SetBinding { 2, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("DFGLUT",                   SetBinding { 3, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .AddBinding("EnvironmentTexture",       SetBinding { 3, 1 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .AddBinding("IrradianceTexture",        SetBinding { 3, 2 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            );

    // Base Color:
    //  When material is metal, the base color is the specific measured reflectance value at normal incidence (F0).
    //  When material is non-metal, the base color represents the reflected diffuse color of the material. A linear 4% is used.

    // opaque pbr technique
    techniqueOpaque = SlimPtr<Technique>();
    techniqueOpaque->AddPass(RenderQueue::Opaque, pipelinePbrDesc.SetName("PBR Opaque"));

    // mask pbr technique
    techniqueMask = SlimPtr<Technique>();
    techniqueMask->AddPass(RenderQueue::AlphaTest, pipelinePbrDesc.SetName("PBR Mask"));

    // blend pbr technique
    techniqueBlend = SlimPtr<Technique>();
    techniqueBlend->AddPass(RenderQueue::Transparent, pipelinePbrDesc.SetName("PBR Blend"));
}

void GLTFViewer::LoadModel() {
    root = builder->CreateNode("root");

    model = SlimPtr<gltf::Model>();
    model->Load(builder, GetUserAsset("Objects/DamagedHelmet/glTF/DamagedHelmet.gltf"), true);
    // model->Load(builder, "/Users/tcheng/Downloads/glTF-Sample-Models/2.0/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf", true);
    // model->Load(builder, "/Users/tcheng/Downloads/glTF-Sample-Models/2.0/WaterBottle/glTF/WaterBottle.gltf", true);
    ProcessModel(model);

    // using the first scene in the model
    root->AddChild(model->GetScene(0));
    root->ApplyTransform();
}

void GLTFViewer::ProcessModel(gltf::Model* model) {
    // updating textures for each material (as we are not using descriptor indexing)
    for (auto &material: model->materials) {
        const gltf::MaterialData& data = material->GetData<gltf::MaterialData>();

        switch (data.alphaMode) {
            case gltf::AlphaMode::Opaque: material->SetTechnique(techniqueOpaque); break;
            case gltf::AlphaMode::Mask:   material->SetTechnique(techniqueMask);   break;
            case gltf::AlphaMode::Blend:  material->SetTechnique(techniqueBlend);  break;
        }

        // material factors
        material->SetUniformBuffer("MaterialFactors", data);

        // base color
        if (data.baseColorTexture >= 0 && data.baseColorSampler >= 0) {
            material->SetTexture("BaseColorTexture",
                    model->images[data.baseColorTexture],
                    model->samplers[data.baseColorSampler]);
        }

        // emissive
        if (data.emissiveTexture >= 0 && data.emissiveSampler >= 0) {
            material->SetTexture("EmissiveTexture",
                    model->images[data.emissiveTexture],
                    model->samplers[data.emissiveSampler]);
        }

        // normal
        if (data.normalTexture >= 0 && data.normalSampler >= 0) {
            material->SetTexture("NormalTexture",
                    model->images[data.normalTexture],
                    model->samplers[data.normalSampler]);
        }

        // occlusion
        if (data.occlusionTexture >= 0 && data.occlusionSampler >= 0) {
            material->SetTexture("OcclusionTexture",
                    model->images[data.occlusionTexture],
                    model->samplers[data.occlusionSampler]);
        }

        // metallic roughness
        if (data.metallicRoughnessTexture >= 0 && data.metallicRoughnessSampler >= 0) {
            material->SetTexture("MetallicRoughnessTexture",
                    model->images[data.metallicRoughnessTexture],
                    model->samplers[data.metallicRoughnessSampler]);
        }

        // attaching lut + env to materials
        material->SetTexture("DFGLUT", dfglut, dfglutSampler);
        material->SetTexture("EnvironmentTexture", skybox->skybox, specularSampler);
        material->SetTexture("IrradianceTexture", skybox->irradiance, diffuseSampler);
    }
}
