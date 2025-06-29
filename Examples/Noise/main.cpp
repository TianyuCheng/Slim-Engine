#include <slim/slim.hpp>

using namespace glm;
using namespace slim;

enum class NoiseType : uint32_t {
    White = 0,
    Value = 1,
    Perlin = 2,
    Simplex = 3,
    Voronoi = 4,
    Layered = 5,
};

struct Screen {
    uvec2 resolution;
    float time;
} screen;

struct Control {
    uint noiseType;
    uint dimension;
    float cellSize;
} control;

int main() {
    slim::Initialize();

    // create a device
    auto context = SlimPtr<Context>(
        ContextDesc()
            .Verbose(true)
            .EnableCompute(true)
            .EnableGraphics(true)
            .EnableValidation(true)
            .EnableGLFW(true)
            .EnableNonSolidPolygonMode()
    );

    // create a device
    auto device = SlimPtr<Device>(context);

    // create a window
    auto window = SlimPtr<Window>(
        device,
        WindowDesc()
            .SetResolution(640, 640)
            .SetResizable(true)
            .SetTitle("Noise")
    );

    // ui and input
    auto input = SlimPtr<Input>(window);
    auto ui = SlimPtr<DearImGui>(device, window);

    // create shaders
    auto vShader = SlimPtr<spirv::VertexShader>(device, "shaders/noise.spv", "vert");
    auto fShader = SlimPtr<spirv::FragmentShader>(device, "shaders/noise.spv", "frag");

    // create data
    auto screen = Screen {};
    auto control = Control {};
    control.noiseType = static_cast<uint32_t>(NoiseType::Layered);
    control.cellSize = 0.05;
    control.dimension = 1;

    // render
    while (window->IsRunning()) {
        Window::PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();

        // prepare ui rendering component
        ui->Begin();
        {
            ImGui::RadioButton("White Noise",   (int*)&control.noiseType, static_cast<uint32_t>(NoiseType::White));
            ImGui::RadioButton("Value Noise",   (int*)&control.noiseType, static_cast<uint32_t>(NoiseType::Value));
            ImGui::RadioButton("Perlin Noise",  (int*)&control.noiseType, static_cast<uint32_t>(NoiseType::Perlin));
            ImGui::RadioButton("Simplex Noise", (int*)&control.noiseType, static_cast<uint32_t>(NoiseType::Simplex));
            ImGui::RadioButton("Voronoi Noise", (int*)&control.noiseType, static_cast<uint32_t>(NoiseType::Voronoi));
            ImGui::RadioButton("Layered Noise", (int*)&control.noiseType, static_cast<uint32_t>(NoiseType::Layered));

            ImGui::DragFloat("Cell Size", (float*)&control.cellSize, 0.05f, 0.0f, 1.0f);
            ImGui::DragInt("Dimension", (int*)&control.dimension, 1, 1, 3);
        }
        ui->End();

        // rendergraph-based design
        RenderGraph renderGraph(frame);
        {
            auto colorBuffer = renderGraph.CreateResource(frame->GetBackBuffer());

            auto colorPass = renderGraph.CreateRenderPass("render");
            colorPass->SetColor(colorBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            colorPass->Execute([&](const RenderInfo &info) {
                auto pipeline = frame->RequestPipeline(
                    GraphicsPipelineDesc()
                        .SetName("colorPass")
                        .SetVertexShader(vShader)
                        .SetFragmentShader(fShader)
                        .SetViewport(frame->GetExtent())
                        .SetCullMode(VK_CULL_MODE_NONE)
                        .SetPrimitive(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
                        .SetRenderPass(info.renderPass)
                        .SetPipelineLayout(
                            PipelineLayoutDesc()
                                .AddBinding("Screen",  SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                .AddBinding("Control", SetBinding { 0, 1 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
                        )
                    );

                // bind pipeline
                info.commandBuffer->BindPipeline(pipeline);

                screen.resolution.x = frame->GetExtent().width;
                screen.resolution.y = frame->GetExtent().height;
                screen.time = glfwGetTime();

                // bind uniforms
                auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
                descriptor->SetUniformBuffer("Screen", info.renderFrame->RequestUniformBuffer(screen));
                descriptor->SetUniformBuffer("Control", info.renderFrame->RequestUniformBuffer(control));
                info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

                // draw
                info.commandBuffer->Draw(6, 1, 0, 0);
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
