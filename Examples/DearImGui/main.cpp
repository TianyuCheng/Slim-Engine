#include <slim/slim.hpp>

using namespace slim;

int main() {
    slim::Initialize();

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

    // dock builder
    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
    ui->Begin();
    {
        ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
        {
            ImGuiID dockspaceId = ImGui::GetID("Main");
            ImGui::DockSpace(dockspaceId, ImVec2(0.0, 0.0), dockspaceFlags);
            ImGui::DockBuilderRemoveNode(dockspaceId);
            ImGui::DockBuilderAddNode(dockspaceId, dockspaceFlags | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceId, ImVec2(640, 480));

            // split into 2 nodes
            auto leftDockId = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.2f, nullptr, &dockspaceId);
            auto downDockId = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Down, 0.25f, nullptr, &dockspaceId);

            ImGui::DockBuilderDockWindow("Left", leftDockId);
            ImGui::DockBuilderDockWindow("Down", downDockId);
            ImGui::DockBuilderDockWindow("Dear ImGui Demo", dockspaceId);
            ImGui::DockBuilderFinish(dockspaceId);
        }
        ImGui::End();
    }
    ui->End();

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
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::SetNextWindowPos(viewport->Pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(viewport->Size, ImGuiCond_Always);
            ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
            {
                ImGuiID dockspaceId = ImGui::GetID("Main");
                ImGui::DockSpace(dockspaceId, ImVec2(0.0, 0.0), dockspaceFlags);
            }
            ImGui::End();

            ImGui::Begin("Left");
            {
                ImGui::Text("Left Content");
            }
            ImGui::End();

            ImGui::Begin("Down");
            {
                ImGui::Text("Down Content");
            }
            ImGui::End();

            ImGui::ShowDemoWindow();
        }
        ui->End();

        graph.Execute();
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
