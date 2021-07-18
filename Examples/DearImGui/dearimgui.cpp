#include <slim/slim.hpp>

using namespace slim;

int main() {
    // create a vulkan context
    auto context = SlimPtr<Context>(
        ContextDesc()
            .EnableCompute(true)
            .EnableGraphics(true)
            .EnableValidation(true),
        WindowDesc()
            .SetResolution(640, 480)
            .SetResizable(true)
            .SetTitle("Dear ImGui")
    );

    // create ui handle
    auto ui = SlimPtr<DearImGui>(context);

    // window
    auto window = context->GetWindow();
    while (!window->ShouldClose()) {
        // query image from swapchain
        auto frame = window->AcquireNext();

        ui->Begin();
        {
            ImGui::ShowDemoWindow();
        }
        ui->End();

        // rendergraph-based design
        RenderGraph graph(frame);
        {
            auto backbuffer = graph.CreateResource(frame->GetBackBuffer());

            auto uiPass = graph.CreateRenderPass("ui");
            uiPass->SetColor(backbuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            uiPass->Execute([=](const RenderGraph &graph) {
                auto commandBuffer = graph.GetGraphicsCommandBuffer();
                ui->Draw(commandBuffer);
            });
        }
        graph.Execute();

        // window update
        window->PollEvents();
    }

    context->WaitIdle();
    return EXIT_SUCCESS;
}
