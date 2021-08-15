#include <slim/slim.hpp>

using namespace slim;

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
            .SetTitle("Dear ImGui")
    );

    // create ui handle
    auto ui = SlimPtr<DearImGui>(device, window, []() {
        DearImGui::EnableDocking();
        DearImGui::EnableMultiview();
    });

    // window
    while (window->IsRunning()) {
        Window::PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();

        // rendergraph-based design
        RenderGraph graph(frame);
        {
            auto backBuffer = graph.CreateResource(frame->GetBackBuffer());

            auto uiPass = graph.CreateRenderPass("ui");
            uiPass->SetColor(backBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            uiPass->Execute([=](const RenderInfo &info) {
                ui->Draw(info.commandBuffer);
            });
        }

        ui->Begin();
        {
            ImGui::ShowDemoWindow();

            ImGui::Begin("Window");
            {
                ImGui::Text("Hello");
            }
            ImGui::End();
        }
        ui->End();

        graph.Execute();
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
