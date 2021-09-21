#include <slim/slim.hpp>
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
            .EnableValidation(true)
            .EnableGLFW(true)
            // additional features
            .EnableRayTracing()
            .EnableBufferDeviceAddress()
            .EnableShaderInt64()
    );

    // create a slim device
    auto device = SlimPtr<Device>(context);

    // create a slim window
    auto window = SlimPtr<Window>(
        device,
        WindowDesc()
            .SetResolution(640, 480)
            .SetResizable(true)
            .SetTitle("Hybrid")
            .EnableFPS(true)
    );

    // ui and input control
    auto ui = SlimPtr<DearImGui>(device, window);
    auto input = SlimPtr<Input>(window);
    auto time = SlimPtr<Time>();

    // camera
    auto camera = SlimPtr<Flycam>("camera");
    camera->SetWalkSpeed(20.0f);
    camera->LookAt(glm::vec3(3.0, 135.0, 0.0), glm::vec3(0.0, 135.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

    // scene
    auto scene = MainScene(device);

    // light
    auto dirLight = DirectionalLight {
        glm::vec4(0.0, -1.0, 0.0, 0.0), // direction
        glm::vec4(1.0, 1.0, 1.0, 1.0),  // light color
    };

    // render
    while (window->IsRunning()) {
        Window::PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();

        // configure camera
        camera->Perspective(1.05, frame->GetAspectRatio(), 0.1, 2000.0f);
        camera->SetExtent(frame->GetExtent());
        camera->Update(input, time);

        // update time
        time->Update();

        // rendergraph-based design
        RenderGraph renderGraph(frame);
        {
            auto colorBuffer = renderGraph.CreateResource(frame->GetBackBuffer());
            auto albedoBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);
            auto normalBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R8G8B8A8_SNORM, VK_SAMPLE_COUNT_1_BIT);
            auto positionBuffer = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT);
            auto depthStencil = renderGraph.CreateResource(frame->GetExtent(), VK_FORMAT_D24_UNORM_S8_UINT, VK_SAMPLE_COUNT_1_BIT);

            // draw gbuffer
            auto gbufferPass = renderGraph.CreateRenderPass("gbuffer");
            gbufferPass->SetColor(albedoBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            gbufferPass->SetColor(normalBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            gbufferPass->SetColor(positionBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            gbufferPass->SetDepthStencil(depthStencil, ClearValue(1.0f, 0));
            gbufferPass->Execute([&](const RenderInfo& info) {
                scene.Draw(info, camera);
            });

            // hybrid ray tracer / rasterizer
            auto raytracePass = renderGraph.CreateComputePass("raytrace");
            raytracePass->SetStorage(albedoBuffer);
            raytracePass->SetStorage(normalBuffer);
            raytracePass->SetStorage(positionBuffer);
            raytracePass->Execute([&](const RenderInfo& info) {
                // ray trace here
                scene.RayTrace(info, dirLight,
                    albedoBuffer->GetImage(),
                    normalBuffer->GetImage(),
                    positionBuffer->GetImage()
                );
            });

            // draw ui
            auto uiPass = renderGraph.CreateRenderPass("ui");
            uiPass->SetColor(colorBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            uiPass->SetTexture(albedoBuffer);
            uiPass->SetTexture(normalBuffer);
            uiPass->SetTexture(positionBuffer);
            uiPass->SetTexture(depthStencil);
            uiPass->Execute([&](const RenderInfo& info) {
                // compose final image here
                scene.Compose(info, dirLight,
                    albedoBuffer->GetImage(),
                    normalBuffer->GetImage(),
                    positionBuffer->GetImage()
                );

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
