#include "fairy.h"
#include "scene.h"

using namespace slim;

int main() {
    slim::Initialize();

    // create a slim device
    auto context = SlimPtr<Context>(
        ContextDesc()
            .Verbose(true)
            .EnableCompute(true)
            .EnableGraphics(true)
            .EnableValidation(false)
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
            .SetTitle("Deferred")
            .EnableFPS(true)
    );

    // ui and input control
    auto ui = SlimPtr<DearImGui>(device, window);
    auto input = SlimPtr<Input>(window);
    auto time = SlimPtr<Time>();

    // scene
    auto fairies = Fairies(device);
    auto mainScene = MainScene(device);

    // camera
    auto camera = SlimPtr<Flycam>("camera");
    camera->SetWalkSpeed(20.0f);
    camera->LookAt(glm::vec3(3.0, 135.0, 0.0), glm::vec3(0.0, 135.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

    // render
    while (window->IsRunning()) {
        Window::PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();

        // camera
        camera->SetExtent(frame->GetExtent());
        camera->Update(input, time);
        camera->Perspective(1.05, frame->GetAspectRatio(), 0.1, 2000.0);

        // fairies
        fairies.UpdateInstances(time);

        // time
        time->Update();

        // rendergraph-based design
        RenderGraph renderGraph(frame);
        {
            auto colorBuffer = renderGraph.CreateResource(frame->GetBackBuffer());
            auto maskBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);
            auto albedoBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);
            auto normalBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_SNORM, VK_SAMPLE_COUNT_1_BIT);
            auto positionBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT);
            auto depthStencil = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_D24_UNORM_S8_UINT, VK_SAMPLE_COUNT_1_BIT);

            // drawing a mask for deferred rendering
            auto maskPass = renderGraph.CreateRenderPass("mask");
            maskPass->SetColor(maskBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            maskPass->Execute([&](const RenderInfo& info) {
                fairies.DrawMask(info, camera);
            });

            auto deferredPass = renderGraph.CreateRenderPass("deferred");
            {
                // gbuffer subpass
                auto gbufferSubpass = deferredPass->CreateSubpass();
                gbufferSubpass->SetColor(colorBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
                gbufferSubpass->SetColor(albedoBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
                gbufferSubpass->SetColor(normalBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
                gbufferSubpass->SetColor(positionBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
                gbufferSubpass->SetDepthStencil(depthStencil, ClearValue(1.0f, 0));
                gbufferSubpass->Execute([&](const RenderInfo &info) {
                    mainScene.DrawGBuffer(info, camera);
                });

                // light subpass
                auto lightSubpass = deferredPass->CreateSubpass();
                lightSubpass->SetColor(colorBuffer);
                lightSubpass->SetInput(albedoBuffer);
                lightSubpass->SetInput(normalBuffer);
                lightSubpass->SetInput(positionBuffer);
                lightSubpass->Execute([&](const RenderInfo &info) {
                    fairies.DrawLight(info, camera,
                        albedoBuffer->GetImage(),
                        normalBuffer->GetImage(),
                        positionBuffer->GetImage()
                    );
                });
            }

            // drawing fairies for debugging
            auto fairyPass = renderGraph.CreateRenderPass("fairies");
            fairyPass->SetColor(colorBuffer);
            fairyPass->SetDepthStencil(depthStencil);
            fairyPass->Execute([&](const RenderInfo& info) {
                fairies.DrawFairy(info, camera);
            });

            // draw ui
            auto uiPass = renderGraph.CreateRenderPass("ui");
            uiPass->SetColor(colorBuffer);
            uiPass->SetTexture(maskBuffer);
            uiPass->SetTexture(albedoBuffer);
            uiPass->SetTexture(normalBuffer);
            uiPass->SetTexture(positionBuffer);
            uiPass->SetTexture(depthStencil);
            uiPass->Execute([&](const RenderInfo &info) {
                ImTextureID mask = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), maskBuffer->GetImage()->AsTexture());
                ImTextureID albedo = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), albedoBuffer->GetImage()->AsTexture());
                ImTextureID normal = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), normalBuffer->GetImage()->AsTexture());
                ImTextureID position = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), positionBuffer->GetImage()->AsTexture());
                ImTextureID depth = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), depthStencil->GetImage()->AsDepthBuffer());

                ui->Begin();
                {
                    ImVec2 winsize = ImVec2(200, 500);
                    ImGui::SetNextWindowSize(winsize, ImGuiCond_Once);
                    ImGui::Begin("GBuffer");
                    {
                        ImVec2 region = ImGui::GetContentRegionAvail();
                        ImVec2 size = ImVec2(region.x, region.x / frame->GetAspectRatio());
                        ImGui::Image(mask, size);
                        ImGui::Image(albedo, size);
                        ImGui::Image(normal, size);
                        ImGui::Image(position, size);
                        ImGui::Image(depth, size);
                    }
                    ImGui::End();
                }
                ui->End();
                ui->Draw(info.commandBuffer);
            });
        }
        renderGraph.Execute();
        input->Reset();
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
