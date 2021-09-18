#include <slim/slim.hpp>

using namespace slim;

// https://github.com/nidorx/matcaps
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
            .SetTitle("MatCap")
    );

    // create vertex and fragment shaders
    auto vShader = SlimPtr<spirv::VertexShader>(device, "main", "shaders/matcap.vert.spv");
    auto fShader = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/matcap.frag.spv");

    // create technique
    auto technique = SlimPtr<Technique>();
    technique->AddPass(RenderQueue::Opaque,
        GraphicsPipelineDesc()
            .SetName("textured")
            .AddVertexBinding(0, sizeof(gltf::Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
                { 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(gltf::Vertex, position)) },
                { 1, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(gltf::Vertex, normal  )) },
             })
            .SetVertexShader(vShader)
            .SetFragmentShader(fShader)
            .SetCullMode(VK_CULL_MODE_BACK_BIT)
            .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .SetDepthTest(VK_COMPARE_OP_LESS)
            .SetPipelineLayout(PipelineLayoutDesc()
                .AddBinding("Camera", SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
                .AddBinding("Model",  SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("MatCap", SetBinding { 2, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)));

    // load matcap texture
    SmartPtr<GPUImage> matcapJade = nullptr;
    SmartPtr<GPUImage> matcapMetal = nullptr;
    SmartPtr<GPUImage> matcapCrystal = nullptr;
    SmartPtr<Sampler> matcapSampler = SlimPtr<Sampler>(device, SamplerDesc { });
    device->Execute([&](CommandBuffer* commandBuffer) {
        matcapJade = TextureLoader::Load2D(commandBuffer, ToAssetPath("Pictures/matcap_jade.png"));
        matcapMetal = TextureLoader::Load2D(commandBuffer, ToAssetPath("Pictures/matcap_metal.jpg"));
        matcapCrystal = TextureLoader::Load2D(commandBuffer, ToAssetPath("Pictures/matcap_crystal.jpg"));
    });

    // model loading
    auto builder = SlimPtr<scene::Builder>(device);
    auto model = gltf::Model { };
    model.Load(builder, ToAssetPath("Characters/Suzanne/glTF/Suzanne.gltf"));
    model.GetScene(0)->ApplyTransform();
    builder->Build();

    // update material
    for (auto& material : model.materials) {
        material->SetTechnique(technique);
    }

    // update matcap
    auto updateMatCap = [&](GPUImage* image) {
        for (auto& material : model.materials) {
            material->SetTexture("MatCap", image, matcapSampler);
        }
    };
    updateMatCap(matcapCrystal);

    auto ui = SlimPtr<DearImGui>(device, window);
    auto input = SlimPtr<Input>(window);

    // camera
    auto camera = SlimPtr<Arcball>("camera");
    camera->LookAt(glm::vec3(0.0, 0.0, 3.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

    // render
    while (window->IsRunning()) {
        Window::PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();

        // controller
        camera->SetExtent(window->GetExtent());
        camera->Update(input);
        camera->Perspective(1.05, frame->GetAspectRatio(), 0.1, 20.0f);

        // sceneFilter result + sorting
        auto culling = SlimPtr<CPUCulling>();
        culling->Cull(model.GetScene(0), camera);
        culling->Sort(RenderQueue::Geometry,    RenderQueue::GeometryLast, SortingOrder::FrontToback);
        culling->Sort(RenderQueue::Transparent, RenderQueue::Transparent,  SortingOrder::BackToFront);

        ui->Begin();
        {

            if (ImGui::Button("Jade")) {
                updateMatCap(matcapJade);
            }

            if (ImGui::Button("Metal")) {
                updateMatCap(matcapMetal);
            }

            if (ImGui::Button("Crystal")) {
                updateMatCap(matcapCrystal);
            }
        }
        ui->End();

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
