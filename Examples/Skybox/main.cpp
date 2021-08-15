#include <slim/slim.hpp>

using namespace slim;

struct RefractProperties {
    float iota;
};

int main() {
    // create a slim device
    auto context = SlimPtr<Context>(
        ContextDesc()
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
    auto vShaderSkybox = SlimPtr<spirv::VertexShader>(device, "main", "shaders/skybox.vert.spv");
    auto fShaderSkybox = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/skybox.frag.spv");

    auto vShaderCommon = SlimPtr<spirv::VertexShader>(device, "main", "shaders/common.vert.spv");
    auto fShaderReflect = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/reflect.frag.spv");
    auto fShaderRefract = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/refract.frag.spv");

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
                .AddBinding("Camera", 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("Model",  1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("Skybox", 2, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)));

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
                .AddBinding("Camera", 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("Model",  1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("Skybox", 2, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)));

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
                .AddBinding("Camera", 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("Model",  1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("Skybox", 2, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .AddBinding("Refract", 2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,        VK_SHADER_STAGE_FRAGMENT_BIT)));

    SmartPtr<Material> skyboxMaterial = nullptr;
    SmartPtr<Material> reflectMaterial = nullptr;
    SmartPtr<Material> refractMaterial = nullptr;

    SmartPtr<Mesh> skyboxMesh = nullptr;
    SmartPtr<Mesh> geomMesh = nullptr;
    SmartPtr<GPUImage> cubemap = nullptr;
    SmartPtr<Sampler> sampler = nullptr;
    SmartPtr<SceneManager> manager = nullptr;
    SmartPtr<Scene> root = nullptr;
    SmartPtr<Scene> skybox = nullptr;
    SmartPtr<Scene> geometry = nullptr;
    uint32_t geometryIndexCount = 0;
    device->Execute([&](CommandBuffer* commandBuffer) {
        cubemap = TextureLoader::LoadCubemap(commandBuffer,
                ToAssetPath("Skyboxes/NiagaraFalls/posx.jpg"),
                ToAssetPath("Skyboxes/NiagaraFalls/negx.jpg"),
                ToAssetPath("Skyboxes/NiagaraFalls/posy.jpg"),
                ToAssetPath("Skyboxes/NiagaraFalls/negy.jpg"),
                ToAssetPath("Skyboxes/NiagaraFalls/posz.jpg"),
                ToAssetPath("Skyboxes/NiagaraFalls/negz.jpg"));

        sampler = SlimPtr<Sampler>(device, SamplerDesc());

        skyboxMaterial = SlimPtr<Material>(device, techniqueSkybox);
        skyboxMaterial->SetTexture("Skybox", cubemap, sampler);

        reflectMaterial = SlimPtr<Material>(device, techniqueReflect);
        reflectMaterial->SetTexture("Skybox", cubemap, sampler);

        refractMaterial = SlimPtr<Material>(device, techniqueRefract);
        refractMaterial->SetTexture("Skybox", cubemap, sampler);
        refractMaterial->SetUniform("Refract", refract.iota);

        auto skyboxData = Cube { };
        skyboxData.ccw = false;
        auto skyboxMeshData = skyboxData.Create();
        skyboxMesh = SlimPtr<Mesh>();
        skyboxMesh->SetVertexAttrib(commandBuffer, skyboxMeshData.vertices, 0);
        skyboxMesh->SetIndexAttrib(commandBuffer, skyboxMeshData.indices);

        auto geomData = Cube { };
        auto geomMeshData = geomData.Create();
        geomMesh = SlimPtr<Mesh>();
        geomMesh->SetVertexAttrib(commandBuffer, geomMeshData.vertices, 0);
        geomMesh->SetIndexAttrib(commandBuffer, geomMeshData.indices);
        geometryIndexCount = geomMeshData.indices.size();

        manager = SlimPtr<SceneManager>();
        root = manager->Create<Scene>("root");

        skybox = manager->Create<Scene>("skybox", root);
        skybox->SetDraw(skyboxMesh, skyboxMaterial, DrawIndexed {
            static_cast<uint32_t>(skyboxMeshData.indices.size()), 1, 0, 0, 0
        });

        geometry = manager->Create<Scene>("geometry", root);
        geometry->SetDraw(geomMesh, reflectMaterial, DrawIndexed {
            geometryIndexCount, 1, 0, 0, 0
        });
    });

    Arcball arcball;
    auto input = SlimPtr<Input>(window);

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
                geometry->SetDraw(geomMesh, reflectMaterial, DrawIndexed {
                    geometryIndexCount, 1, 0, 0, 0
                });
            }

            if (selected == 1) {
                geometry->SetDraw(geomMesh, refractMaterial, DrawIndexed {
                    geometryIndexCount, 1, 0, 0, 0
                });
            }

            ImGui::End();
        }
        ui->End();

        // create a camera
        auto camera = SlimPtr<Camera>("camera");
        camera->Perspective(1.05, frame->GetAspectRatio(), 0.1, 20.0f);
        camera->LookAt(glm::vec3(0.0, 0.0, 3.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

        // update
        arcball.SetExtent(frame->GetExtent());
        arcball.Update(input);

        // transform scene nodes
        geometry->SetTransform(arcball.GetTransform());
        root->Update();

        // sceneFilter result + sorting
        auto culling = SlimPtr<CPUCulling>();
        culling->Cull(root, camera);
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
                renderer.Draw(camera, culling->GetDrawables(RenderQueue::Geometry, RenderQueue::GeometryLast));
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
