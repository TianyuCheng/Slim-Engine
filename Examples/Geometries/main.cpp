#include <slim/slim.hpp>

using namespace slim;

struct Geometries {
    Plane    plane;
    Cube     cube;
    Sphere   sphere;
    Cone     cone;
    Cylinder cylinder;
};

template <typename T>
Scene* CreateGeometry(SceneManager* sceneMgr, CommandBuffer* commandBuffer, Material* material, const T& geometry) {
    GeometryData data = geometry.Create();
    auto mesh = SlimPtr<Mesh>();
    mesh->SetVertexAttrib(commandBuffer, data.vertices, 0);
    mesh->SetIndexAttrib(commandBuffer, data.indices);

    auto scene = sceneMgr->Create<Scene>("geometry");
    scene->SetMaterial(material);
    scene->SetMesh(mesh);
    scene->SetDraw(DrawIndexed { static_cast<uint32_t>(data.indices.size()), 1, 0, 0, 0 });
    return scene;
}

int main() {
    // create a slim device
    auto context = SlimPtr<Context>(
        ContextDesc()
            .EnableCompute(true)
            .EnableGraphics(true)
            .EnableValidation(true)
            .EnableGLFW(true)
            .EnableNonSolidPolygonMode()
    );

    // create a slim device
    auto device = SlimPtr<Device>(context);

    // create a slim window
    auto window = SlimPtr<Window>(
        device,
        WindowDesc()
            .SetResolution(640, 480)
            .SetResizable(true)
            .SetTitle("Geometries")
    );

    // create vertex and fragment shaders
    auto vShader = SlimPtr<spirv::VertexShader>(device, "main", "shaders/simple.vert.spv");
    auto fShader = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/simple.frag.spv");

    // create technique
    auto technique = SlimPtr<Technique>();
    technique->AddPass(RenderQueue::Opaque,
        GraphicsPipelineDesc()
            .SetName("textured")
            .AddVertexBinding(0, sizeof(GeometryData::Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
                { 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryData::Vertex, position) },
                { 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryData::Vertex, normal)   },
                { 2, VK_FORMAT_R32G32_SFLOAT,    offsetof(GeometryData::Vertex, texcoord) },
             })
            .SetVertexShader(vShader)
            .SetFragmentShader(fShader)
            .SetCullMode(VK_CULL_MODE_NONE)
            .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .SetDepthTest(VK_COMPARE_OP_LESS)
            .SetPolygonMode(VK_POLYGON_MODE_LINE)
            .SetPipelineLayout(PipelineLayoutDesc()
                .AddBinding("Camera", 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("Model",  1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)));

    // create the first material
    auto material = SlimPtr<Material>(device, technique);

    // create scene with meshes
    auto sceneMgr = SlimPtr<SceneManager>();
    SmartPtr<Scene> node = nullptr;

    Geometries geometries;
    geometries.plane = Plane { 1.0f, 1.0f, 6, 8, true };
    geometries.cube = Cube { 1.0f, 1.0f, 1.0f, 6, 8, 10, true };
    geometries.sphere = Sphere { 1.0f, 16, 8 };
    geometries.cone = Cone { 1.0f, 1.0f, 16, 8 };
    geometries.cylinder = Cylinder { 1.0f, 1.0f, 1.0f, 8, 2 };

    // ui options
    uint32_t selectedGeometry = 3;
    std::vector<const char*> geometryNames = { "Plane", "Cube", "Geometry", "Cone", "Cylinder" };
    auto ui = SlimPtr<DearImGui>(device, window);

    // render
    while (!window->ShouldClose()) {
        // query image from swapchain
        auto frame = window->AcquireNext();

        // create a camera
        auto camera = SlimPtr<Camera>("camera");
        camera->LookAt(glm::vec3(0.0, 0.0, 5.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
        camera->Perspective(1.05, frame->GetAspectRatio(), 0.1, 20.0f);

        // show gui
        bool changed = false;
        ui->Begin();
        {
            ImGui::Begin("Geometry");
            {
                if (ImGui::BeginCombo("Geometry", geometryNames[selectedGeometry])) {
                    for (uint32_t i = 0; i < geometryNames.size(); i++) {
                        bool selected = i == selectedGeometry;
                        if (ImGui::Selectable(geometryNames[i], selected)) {
                            selectedGeometry = i;
                            changed = true;
                        }
                        if (selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }
            ImGui::End();
        }
        ui->End();

        if (changed || node == nullptr) {
            device->Execute([&](CommandBuffer* commandBuffer) {
                switch (selectedGeometry) {
                    case 0: node = CreateGeometry(sceneMgr, commandBuffer, material, geometries.plane);    break;
                    case 1: node = CreateGeometry(sceneMgr, commandBuffer, material, geometries.cube);     break;
                    case 2: node = CreateGeometry(sceneMgr, commandBuffer, material, geometries.sphere);   break;
                    case 3: node = CreateGeometry(sceneMgr, commandBuffer, material, geometries.cone);     break;
                    case 4: node = CreateGeometry(sceneMgr, commandBuffer, material, geometries.cylinder); break;
                }
            });
        }

        // transform scene nodes
        node->Update();

        // sceneFilter result + sorting
        auto culling = SlimPtr<CPUCulling>();
        culling->Cull(node, camera);
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

        // window update
        window->PollEvents();
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
