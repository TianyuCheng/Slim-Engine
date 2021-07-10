#include <slim/slim.hpp>
#include "Time.h"

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
            .SetTitle("Slim Application")
    );

    // create vertex and index buffers
    auto vBuffer = SlimPtr<VertexBuffer>(context.get(), 256);
    auto iBuffer = SlimPtr<IndexBuffer>(context.get(), 256);
    context->Execute([=](CommandBuffer *commandBuffer) {
        // prepare vertex data
        std::vector<float> positions = {
            0.0f, 0.0f, 0.5f,
            1.0f, 0.0f, 0.5f,
            1.0f, 1.0f, 0.5f,
        };
        std::vector<uint32_t> indices = {
            0, 1, 2
        };

        commandBuffer->CopyDataToBuffer(positions, vBuffer.get());
        commandBuffer->CopyDataToBuffer(indices, iBuffer.get());
    });

    // create vertex and fragment shaders
    auto vShader = SlimPtr<spirv::VertexShader>(context.get(), "main", "shaders/simple_vertex.vert.spv");
    auto fShader = SlimPtr<spirv::FragmentShader>(context.get(), "main", "shaders/simple_fragment.frag.spv");

    // create ui handle
    auto ui = SlimPtr<DearImGui>(context.get());
    uint32_t index = 0;
    FPS fps;

    // window
    auto window = context->GetWindow();
    while (!window->ShouldClose()) {
        fps.Update();

        // window update
        window->PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();

        // glm::vec3 lightDirection = glm::vec3(-1.0f, -1.0f, -1.0f);
        // glm::vec3 eyePosition = glm::vec3();
        //
        // glm::mat4 shadowMVP;
        // glm::mat4 colorMVP;

        // rendergraph-based design
        RenderGraph graph(frame);
        {
            auto backbuffer = graph.CreateResource(frame->GetBackbuffer());
            // auto shadowAtlas = graph.CreateResource(VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, frame->GetExtent());
            // auto depthBuffer = graph.CreateResource(VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, frame->GetExtent());
            //
            // auto shadowPass = graph.CreateRenderPass("shadow");
            // shadowPass->SetDepthStencil(shadowAtlas, ClearValue(1.0f));
            // shadowPass->Execute([=](const RenderGraph &graph) {
            //     auto renderFrame = graph.GetRenderFrame();
            //     auto commandBuffer = graph.GetGraphicsCommandBuffer();
            //     auto pipeline = renderFrame->RequestPipeline("shadowPass", GraphicsPipelineDesc());
            //
            //     auto descriptor = SlimPtr<Descriptor>(pipeline);
            //     // descriptor->SetUniform("mvp", renderFrame->RequestUniformBuffer<glm::mat4>(shadowMVP));
            //
            //     commandBuffer->BindPipeline(pipeline);
            //     commandBuffer->BindDescriptor(descriptor.get());
            //     commandBuffer->BindVertexBuffer(0, vBuffer.get(), 0);
            //     commandBuffer->BindIndexBuffer(iBuffer.get());
            //     commandBuffer->DrawIndexed(3, 1, 0, 0, 0);
            // });
            //
            // auto colorPass = graph.CreateRenderPass("color");
            // colorPass->SetTexture(shadowAtlas);
            // colorPass->SetColor(backbuffer, ClearValue(0.0, 0.0f, 0.0f, 1.0f));
            // colorPass->SetDepthStencil(depthBuffer, ClearValue(0.0f));
            // colorPass->Execute([=](const RenderGraph &graph) {
            //     auto renderFrame = graph.GetRenderFrame();
            //     auto commandBuffer = graph.GetGraphicsCommandBuffer();
            //     auto pipeline = renderFrame->RequestPipeline("colorPass", GraphicsPipelineDesc());
            //
            //     auto descriptor = SlimPtr<Descriptor>(pipeline);
            //     // descriptor->SetUniform("mvp", renderFrame->RequestUniformBuffer<glm::mat4>(colorMVP));
            //
            //     commandBuffer->BindPipeline(pipeline);
            //     commandBuffer->BindDescriptor(descriptor.get());
            //     commandBuffer->BindVertexBuffer(0, vBuffer.get(), 0);
            //     commandBuffer->BindIndexBuffer(iBuffer.get());
            //     commandBuffer->DrawIndexed(3, 1, 0, 0, 0);
            // });

            auto uiPass = graph.CreateRenderPass("ui");
            uiPass->SetColor(backbuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            uiPass->Execute([=](const RenderGraph &graph) {
                auto commandBuffer = graph.GetGraphicsCommandBuffer();
                ui->Draw(commandBuffer);
            });
        }

        ui->Begin();
        {
            ImGui::Text("FPS: %f", fps.GetValue());
            ImGui::Begin("Render Graph");
            {
                graph.Visualize();
            }
            ImGui::End();
        }
        ui->End();

        graph.Execute();

        // if (index >= 6) break;
        index++;
    }

    context->WaitIdle();

    return EXIT_SUCCESS;
}
