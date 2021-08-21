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
    scene->SetDraw(mesh, material, DrawIndexed { static_cast<uint32_t>(data.indices.size()), 1, 0, 0, 0 });
    return scene;
}

constexpr uint32_t PLANE    = 0;
constexpr uint32_t CUBE     = 1;
constexpr uint32_t SPHERE   = 2;
constexpr uint32_t CONE     = 3;
constexpr uint32_t CYLINDER = 4;

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

    // create window input
    auto input = SlimPtr<Input>(window);

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
            .SetCullMode(VK_CULL_MODE_BACK_BIT)
            .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .SetDepthTest(VK_COMPARE_OP_LESS)
            // .SetPolygonMode(VK_POLYGON_MODE_LINE)
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
    uint32_t selectedGeometry = CYLINDER;
    std::vector<const char*> geometryNames = { "Plane", "Cube", "Sphere", "Cone", "Cylinder" };
    auto ui = SlimPtr<DearImGui>(device, window);

    // controller
    auto arcball = SlimPtr<Arcball>();
    arcball->SetDamping(0.1);
    arcball->SetSensitivity(1.0);
    arcball->LookAt(glm::vec3(0.0, 0.0, 5.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

    // render
    while (window->IsRunning()) {
        Window::PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();

        // create a camera
        arcball->Perspective(1.05, frame->GetAspectRatio(), 0.1, 20.0f);

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

                // Plane Parameters
                if (selectedGeometry == PLANE) {
                    changed |= ImGui::DragFloat("Width", &geometries.plane.width, 0.1f, 0.5f, 5.0f);
                    changed |= ImGui::DragFloat("Height", &geometries.plane.height, 0.1f, 0.5f, 5.0f);
                    changed |= ImGui::DragInt("Width Segments", (int*)&geometries.plane.widthSegments, 1, 1, 20);
                    changed |= ImGui::DragInt("Height Segments", (int*)&geometries.plane.heightSegments, 1, 1, 20);
                    changed |= ImGui::Checkbox("CCW", &geometries.plane.ccw);
                }

                // Cube Parameters
                if (selectedGeometry == CUBE) {
                    changed |= ImGui::DragFloat("Width", &geometries.cube.width, 0.1f, 0.5f, 5.0f);
                    changed |= ImGui::DragFloat("Height", &geometries.cube.height, 0.1f, 0.5f, 5.0f);
                    changed |= ImGui::DragFloat("Depth", &geometries.cube.depth, 0.1f, 0.5f, 5.0f);
                    changed |= ImGui::DragInt("Width Segments", (int*)&geometries.cube.widthSegments, 1, 1, 20);
                    changed |= ImGui::DragInt("Height Segments", (int*)&geometries.cube.heightSegments, 1, 1, 20);
                    changed |= ImGui::DragInt("Depth Segments", (int*)&geometries.cube.depthSegments, 1, 1, 20);
                    changed |= ImGui::Checkbox("CCW", &geometries.cube.ccw);
                }

                // Sphere Parameters
                if (selectedGeometry == SPHERE) {
                    changed |= ImGui::DragFloat("Radius", &geometries.sphere.radius, 0.1f, 0.5f, 5.0f);
                    changed |= ImGui::DragFloat("Phi Start", &geometries.sphere.phiStart, 0.1f, 0.1f, M_PI);
                    changed |= ImGui::DragFloat("Phi Length", &geometries.sphere.phiLength, 0.1f, 0.1f, M_PI);
                    changed |= ImGui::DragFloat("Theta Start", &geometries.sphere.thetaStart, 0.1f, 0.1f, M_PI * 2.0f);
                    changed |= ImGui::DragFloat("Theta Length", &geometries.sphere.thetaLength, 0.1f, 0.1f, M_PI * 2.0f);
                    changed |= ImGui::DragInt("Radial Segments", (int*)&geometries.sphere.radialSegments, 1, 1, 20);
                    changed |= ImGui::DragInt("Height Segments", (int*)&geometries.sphere.heightSegments, 1, 1, 20);
                    changed |= ImGui::Checkbox("CCW", &geometries.sphere.ccw);
                }

                // Cone Parameters
                if (selectedGeometry == CONE) {
                    changed |= ImGui::DragFloat("Radius", &geometries.cone.radius, 0.1f, 0.5f, 5.0f);
                    changed |= ImGui::DragFloat("Height", &geometries.cone.height, 0.1f, 0.5f, 5.0f);
                    changed |= ImGui::DragFloat("Theta Start", &geometries.cone.thetaStart, 0.1f, 0.1f, M_PI * 2.0f);
                    changed |= ImGui::DragFloat("Theta Length", &geometries.cone.thetaLength, 0.1f, 0.1f, M_PI * 2.0f);
                    changed |= ImGui::DragInt("Radial Segments", (int*)&geometries.cone.radialSegments, 1, 1, 20);
                    changed |= ImGui::DragInt("Height Segments", (int*)&geometries.cone.heightSegments, 1, 1, 20);
                    changed |= ImGui::Checkbox("Open Ended", &geometries.cone.openEnded);
                    changed |= ImGui::Checkbox("CCW", &geometries.cone.ccw);
                }

                // Cylinder Parameters
                if (selectedGeometry == CYLINDER) {
                    changed |= ImGui::DragFloat("Radius Top", &geometries.cylinder.radiusTop, 0.1f, 0.5f, 5.0f);
                    changed |= ImGui::DragFloat("Radius Bottom", &geometries.cylinder.radiusBottom, 0.1f, 0.5f, 5.0f);
                    changed |= ImGui::DragFloat("Height", &geometries.cylinder.height, 0.1f, 0.5f, 5.0f);
                    changed |= ImGui::DragFloat("ThetaStart", &geometries.cylinder.thetaStart, 0.1f, 0.1f, M_PI * 2.0f);
                    changed |= ImGui::DragFloat("ThetaLength", &geometries.cylinder.thetaLength, 0.1f, 0.1f, M_PI * 2.0f);
                    changed |= ImGui::DragInt("Radial Segments", (int*)&geometries.cylinder.radialSegments, 1, 1, 20);
                    changed |= ImGui::DragInt("Height Segments", (int*)&geometries.cylinder.heightSegments, 1, 1, 20);
                    changed |= ImGui::Checkbox("Open Ended", &geometries.cylinder.openEnded);
                    changed |= ImGui::Checkbox("CCW", &geometries.cylinder.ccw);
                }

            }
            ImGui::End();
        }
        ui->End();

        // update scene
        if (changed || node == nullptr) {
            device->Execute([&](CommandBuffer* commandBuffer) {
                switch (selectedGeometry) {
                    case PLANE:    node = CreateGeometry(sceneMgr, commandBuffer, material, geometries.plane);    break;
                    case CUBE:     node = CreateGeometry(sceneMgr, commandBuffer, material, geometries.cube);     break;
                    case SPHERE:   node = CreateGeometry(sceneMgr, commandBuffer, material, geometries.sphere);   break;
                    case CONE:     node = CreateGeometry(sceneMgr, commandBuffer, material, geometries.cone);     break;
                    case CYLINDER: node = CreateGeometry(sceneMgr, commandBuffer, material, geometries.cylinder); break;
                }
            });
            node->SetTransform(arcball->GetModelMatrix());
        }

        // controller
        arcball->SetExtent(window->GetExtent());
        if (arcball->Update(input)) {
            node->SetTransform(arcball->GetModelMatrix());
        }

        // transform scene nodes
        node->Update();

        // sceneFilter result + sorting
        auto culling = SlimPtr<CPUCulling>();
        culling->Cull(node, arcball);
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
