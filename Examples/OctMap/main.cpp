#include <slim/slim.hpp>

using namespace slim;

enum MappingType : uint32_t {
    OCTAHEDRON = 0,
    HEMI_OCTAHEDRON_V1 = 1,
    HEMI_OCTAHEDRON_V2 = 2,
};

struct Controller {
    MappingType type;
};

int main() {
    slim::Initialize();

    // create device
    auto context = SlimPtr<Context>(
        ContextDesc()
            .Verbose(true)
            .EnableCompute(true)
            .EnableGraphics(true)
            .EnableValidation(true)
            .EnableGLFW(true)
    );

    // create device
    auto device = SlimPtr<Device>(context);

    // create window
    auto window = SlimPtr<Window>(
        device,
        WindowDesc()
            .SetResolution(640, 640)
            .SetResizable(true)
            .SetTitle("OctMap")
    );

    // create ui
    auto ui = SlimPtr<DearImGui>(device, window);

    // create shaders
    auto decodeVShader = SlimPtr<spirv::VertexShader>(device, "main", "shaders/decode_hemioct.vert.spv");
    auto decodeFShader = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/decode_hemioct.frag.spv");

    // SmartPtr<GPUImage> image = nullptr;
    SmartPtr<GPUImage> cubemap = nullptr;
    SmartPtr<Sampler> sampler = nullptr;
    device->Execute([&](CommandBuffer* commandBuffer) {
        // image = TextureLoader::Load2D(commandBuffer,
        //         GetUserAsset("Skyboxes/NiagaraFalls/posx.jpg"));
        cubemap = TextureLoader::LoadCubemap(commandBuffer,
                GetUserAsset("Skyboxes/NiagaraFalls/posx.jpg"),
                GetUserAsset("Skyboxes/NiagaraFalls/negx.jpg"),
                GetUserAsset("Skyboxes/NiagaraFalls/posy.jpg"),
                GetUserAsset("Skyboxes/NiagaraFalls/negy.jpg"),
                GetUserAsset("Skyboxes/NiagaraFalls/posz.jpg"),
                GetUserAsset("Skyboxes/NiagaraFalls/negz.jpg"));
        sampler = SlimPtr<Sampler>(device, SamplerDesc());
    });

    Controller controller = {};
    controller.type = OCTAHEDRON;

    // render
    while (window->IsRunning()) {
        Window::PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();

        ui->Begin();
        {
            if (ImGui::Button("Octahedron")) {
                controller.type = OCTAHEDRON;
            }

            if (ImGui::Button("Hemi Octahedron v1")) {
                controller.type = HEMI_OCTAHEDRON_V1;
            }

            if (ImGui::Button("Hemi Octahedron v2")) {
                controller.type = HEMI_OCTAHEDRON_V2;
            }
        }
        ui->End();

        // rendergraph-based design
        RenderGraph renderGraph(frame);
        {
            auto colorBuffer = renderGraph.CreateResource(frame->GetBackBuffer());

            auto decodePass = renderGraph.CreateRenderPass("decode");
            decodePass->SetColor(colorBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            decodePass->Execute([&](const RenderInfo &info) {
                auto pipeline = frame->RequestPipeline(
                    GraphicsPipelineDesc()
                        .SetName("decodePass")
                        .SetVertexShader(decodeVShader)
                        .SetFragmentShader(decodeFShader)
                        .SetViewport(frame->GetExtent())
                        .SetCullMode(VK_CULL_MODE_BACK_BIT)
                        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
                        .SetRenderPass(info.renderPass)
                        .SetPipelineLayout(PipelineLayoutDesc()
                            .AddPushConstant("Info", Range { 0, sizeof(Controller) }, VK_SHADER_STAGE_FRAGMENT_BIT)
                            .AddBinding("Skybox", SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                        )
                    );

                // bind pipeline
                info.commandBuffer->BindPipeline(pipeline);

                // bind descriptor
                auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
                descriptor->SetTexture("Skybox", cubemap, sampler);
                info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

                // push constant
                info.commandBuffer->PushConstants(pipeline->Layout(), "Info", &controller);

                // draw quad
                info.commandBuffer->Draw(6, 1, 0, 0);
            });

            auto uiPass = renderGraph.CreateRenderPass("ui");
            uiPass->SetColor(colorBuffer);
            uiPass->Execute([&](const RenderInfo &info) {
                ui->Draw(info.commandBuffer);
            });
        }
        renderGraph.Execute();
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
