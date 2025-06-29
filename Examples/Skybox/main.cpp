#include <slim/slim.hpp>

using namespace slim;

struct RefractProperties {
    float iota;
};

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
    );

    // create a slim device
    auto device = SlimPtr<Device>(context);

    // create a slim window
    auto window = SlimPtr<Window>(
        device,
        WindowDesc()
            .SetResolution(640, 480)
            .SetResizable(true)
            .SetTitle("Skybox")
    );

    // create vertex and fragment shaders
    auto vShaderSkybox = SlimPtr<spirv::VertexShader>(device, "shaders/skybox.spv");
    auto fShaderSkybox = SlimPtr<spirv::FragmentShader>(device, "shaders/skybox.spv");

    auto vShaderCommon = SlimPtr<spirv::VertexShader>(device, "shaders/common.spv");
    auto fShaderReflect = SlimPtr<spirv::FragmentShader>(device, "shaders/reflect.spv");
    auto fShaderRefract = SlimPtr<spirv::FragmentShader>(device, "shaders/refract.spv");

    RefractProperties refract;
    refract.iota = 1.04;

    // create technique
    auto techniqueSkybox = SlimPtr<Technique>();
    techniqueSkybox->AddPass(RenderQueue::Opaque,
        GraphicsPipelineDesc()
            .SetName("skybox")
            .AddVertexBinding(0, sizeof(GeometryData::Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
                { 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryData::Vertex, position) },
             })
            .SetVertexShader(vShaderSkybox)
            .SetFragmentShader(fShaderSkybox)
            .SetCullMode(VK_CULL_MODE_BACK_BIT)
            .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .SetDepthTest(VK_COMPARE_OP_ALWAYS, false)
            .SetPipelineLayout(PipelineLayoutDesc()
                .AddBinding("Camera", SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("Model",  SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("Skybox", SetBinding { 2, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)));

    auto techniqueReflect = SlimPtr<Technique>();
    techniqueReflect->AddPass(RenderQueue::Opaque,
        GraphicsPipelineDesc()
            .SetName("reflect")
            .AddVertexBinding(0, sizeof(GeometryData::Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
                { 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryData::Vertex, position) },
                { 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryData::Vertex, normal)   },
             })
            .SetVertexShader(vShaderCommon)
            .SetFragmentShader(fShaderReflect)
            .SetCullMode(VK_CULL_MODE_NONE)
            .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .SetDepthTest(VK_COMPARE_OP_LESS)
            .SetPipelineLayout(PipelineLayoutDesc()
                .AddBinding("Camera", SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("Model",  SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("Skybox", SetBinding { 2, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)));

    auto techniqueRefract = SlimPtr<Technique>();
    techniqueRefract->AddPass(RenderQueue::Opaque,
        GraphicsPipelineDesc()
            .SetName("refract")
            .AddVertexBinding(0, sizeof(GeometryData::Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
                { 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryData::Vertex, position) },
                { 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryData::Vertex, normal)   },
             })
            .SetVertexShader(vShaderCommon)
            .SetFragmentShader(fShaderRefract)
            .SetCullMode(VK_CULL_MODE_NONE)
            .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .SetDepthTest(VK_COMPARE_OP_LESS)
            .SetPipelineLayout(PipelineLayoutDesc()
                .AddBinding("Camera",  SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("Model",   SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("Skybox",  SetBinding { 2, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .AddBinding("Refract", SetBinding { 2, 1 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT)));

    SmartPtr<scene::Material> skyboxMaterial = nullptr;
    SmartPtr<scene::Material> reflectMaterial = nullptr;
    SmartPtr<scene::Material> refractMaterial = nullptr;

    SmartPtr<scene::Mesh> skyboxMesh = nullptr;
    SmartPtr<scene::Mesh> geomMesh = nullptr;
    SmartPtr<GPUImage> cubemap = nullptr;
    SmartPtr<Sampler> sampler = nullptr;
    SmartPtr<scene::Builder> builder = nullptr;
    SmartPtr<scene::Node> root = nullptr;
    SmartPtr<scene::Node> skybox = nullptr;
    SmartPtr<scene::Node> geometry = nullptr;
    uint32_t geometryIndexCount = 0;
    device->Execute([&](CommandBuffer* commandBuffer) {
        cubemap = TextureLoader::LoadCubemap(commandBuffer,
                GetUserAsset("Skyboxes/NiagaraFalls/posx.jpg"),
                GetUserAsset("Skyboxes/NiagaraFalls/negx.jpg"),
                GetUserAsset("Skyboxes/NiagaraFalls/posy.jpg"),
                GetUserAsset("Skyboxes/NiagaraFalls/negy.jpg"),
                GetUserAsset("Skyboxes/NiagaraFalls/posz.jpg"),
                GetUserAsset("Skyboxes/NiagaraFalls/negz.jpg"));

        builder = SlimPtr<scene::Builder>(device);

        sampler = SlimPtr<Sampler>(device, SamplerDesc());

        skyboxMaterial = builder->CreateMaterial(techniqueSkybox);
        skyboxMaterial->SetTexture("Skybox", cubemap, sampler);

        reflectMaterial = builder->CreateMaterial(techniqueReflect);
        reflectMaterial->SetTexture("Skybox", cubemap, sampler);

        refractMaterial = builder->CreateMaterial(techniqueRefract);
        refractMaterial->SetTexture("Skybox", cubemap, sampler);
        refractMaterial->SetUniformBuffer("Refract", refract.iota);

        auto skyboxData = Cube { };
        skyboxData.ccw = false;
        auto skyboxMeshData = skyboxData.Create();
        skyboxMesh = builder->CreateMesh();
        skyboxMesh->SetVertexBuffer(skyboxMeshData.vertices);
        skyboxMesh->SetIndexBuffer(skyboxMeshData.indices);

        auto geomData = Cube { };
        auto geomMeshData = geomData.Create();
        geomMesh = builder->CreateMesh();
        geomMesh->SetVertexBuffer(geomMeshData.vertices);
        geomMesh->SetIndexBuffer(geomMeshData.indices);
        geometryIndexCount = geomMeshData.indices.size();

        root = builder->CreateNode("root");

        skybox = builder->CreateNode("skybox", root);
        skybox->SetDraw(skyboxMesh, skyboxMaterial);

        geometry = builder->CreateNode("geometry", root);
        geometry->SetDraw(geomMesh, reflectMaterial);

    });
    builder->Build();

    auto input = SlimPtr<Input>(window);
    auto arcball = SlimPtr<Arcball>();
    arcball->LookAt(glm::vec3(0.0, 0.0, 3.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

    auto ui = SlimPtr<DearImGui>(device, window);

    uint32_t selected = 0;
    std::vector<const char*> materials = { "reflect", "refract" };

    // render
    while (window->IsRunning()) {
        Window::PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();

        ui->Begin();
        {
            ImGui::Begin("Control");

            if (ImGui::BeginCombo("Material", materials[selected])) {
                for (uint32_t i = 0; i < materials.size(); i++) {
                    bool isSelected = i == selected;
                    if (ImGui::Selectable(materials[i], isSelected)) {
                        selected = i;
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (selected == 0) {
                geometry->SetDraw(geomMesh, reflectMaterial);
            }

            if (selected == 1) {
                geometry->SetDraw(geomMesh, refractMaterial);
            }

            ImGui::End();
        }
        ui->End();

        // create a camera
        arcball->Perspective(1.05, frame->GetAspectRatio(), 0.1, 20.0f);

        // update
        arcball->SetExtent(window->GetExtent());
        arcball->Update(input);

        // sceneFilter result + sorting
        auto culling = SlimPtr<CPUCulling>();
        culling->Cull(root, arcball);
        culling->Sort(RenderQueue::Geometry,    RenderQueue::GeometryLast, SortingOrder::FrontToback);
        culling->Sort(RenderQueue::Transparent, RenderQueue::Transparent,  SortingOrder::BackToFront);

        // rendergraph-based design
        RenderGraph renderGraph(frame);
        {
            auto colorBuffer = renderGraph.CreateResource(frame->GetBackBuffer());
            auto depthBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_D24_UNORM_S8_UINT, VK_SAMPLE_COUNT_1_BIT);

            auto colorPass = renderGraph.CreateRenderPass("color");
            colorPass->SetColor(colorBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            colorPass->SetDepthStencil(depthBuffer, ClearValue(1.0f, 0));
            colorPass->Execute([&](const RenderInfo &info) {
                MeshRenderer renderer(info);
                renderer.Draw(arcball, culling->GetDrawables(RenderQueue::Geometry, RenderQueue::GeometryLast));
            });

            auto uiPass = renderGraph.CreateRenderPass("ui");
            uiPass->SetColor(colorBuffer);
            uiPass->Execute([&](const RenderInfo &info) {
                ui->Draw(info.commandBuffer);
            });
        }
        renderGraph.Execute();

        input->Reset();
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
