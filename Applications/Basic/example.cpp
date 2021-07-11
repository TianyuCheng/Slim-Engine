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

    // create vertex and fragment shaders
    auto vShader = SlimPtr<spirv::VertexShader>(context.get(), "main", "shaders/textured_vertex.vert.spv");
    auto fShader = SlimPtr<spirv::FragmentShader>(context.get(), "main", "shaders/textured_fragment.frag.spv");

    // create texture
    SmartPtr<GPUImage2D> texture;
    SmartPtr<Sampler> sampler = SlimPtr<Sampler>(context.get(), SamplerDesc());

    // initialize
    auto renderFrame = SlimPtr<RenderFrame>(context.get());
    auto commandBuffer = renderFrame->RequestCommandBuffer(VK_QUEUE_TRANSFER_BIT);
    commandBuffer->Begin();
    {
        // prepare vertex data
        std::vector<float> positions = {
            // position              texcoords
            -0.5f, -0.5f,  0.5f,     0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,     1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,     1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,     0.0f, 1.0f,

            // position              texcoords
            -0.5f, -0.5f, -0.5f,     0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,     1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,     1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,     0.0f, 1.0f,
        };
        std::vector<uint32_t> indices = {
            0, 1, 2,
            2, 3, 0,

            // 4, 7, 6,
            // 6, 5, 4,
            //
            // 3, 7, 4,
            // 4, 0, 3,
            //
            // 1, 5, 6,
            // 6, 2, 1,
            //
            // 6, 7, 3,
            // 3, 2, 6,
            //
            // 4, 5, 1,
            // 1, 0, 4
        };

        commandBuffer->CopyDataToBuffer(positions, vBuffer.get());
        commandBuffer->CopyDataToBuffer(indices, iBuffer.get());

        texture.reset(TextureLoader::Load2D(commandBuffer, "/Users/tcheng/Pictures/miku.png", VK_FILTER_LINEAR));
    }
    commandBuffer->End();
    commandBuffer->Submit();
    context->WaitIdle();

    // create ui handle
    auto ui = SlimPtr<DearImGui>(context.get());
    FPS fps;
    float angle = 0.0f;

    // window
    auto window = context->GetWindow();
    while (!window->ShouldClose()) {
        // window update
        window->PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();

        fps.Update();
        // angle += 0.01f;

        // rendergraph-based design
        RenderGraph graph(frame);
        {
            auto backbuffer = graph.CreateResource(frame->GetBackbuffer());
            auto depthBuffer = graph.CreateResource(frame->GetExtent(), VK_FORMAT_D24_UNORM_S8_UINT, VK_SAMPLE_COUNT_1_BIT);

            auto colorPass = graph.CreateRenderPass("color");
            colorPass->SetColor(backbuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            colorPass->SetDepthStencil(depthBuffer, ClearValue(1.0f, 0));
            colorPass->Execute([=](const RenderGraph &graph) {
                auto renderFrame = graph.GetRenderFrame();
                auto commandBuffer = graph.GetGraphicsCommandBuffer();
                auto pipeline = renderFrame->RequestPipeline("colorPass",
                    GraphicsPipelineDesc()
                        .AddVertexBinding(0, sizeof(glm::vec3) + sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX)
                        .AddVertexAttrib(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
                        .AddVertexAttrib(0, 1, VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec3))
                        .SetVertexShader(vShader.get())
                        .SetFragmentShader(fShader.get())
                        .SetViewport(frame->GetExtent())
                        .SetCullMode(VK_CULL_MODE_BACK_BIT)
                        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
                        .SetRenderPass(graph.GetRenderPass())
                        .SetDepthTest(VK_COMPARE_OP_LESS)
                        .SetPipelineLayout(PipelineLayoutDesc()
                            .AddBinding("Camera", 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
                            .AddBinding("Albedo", 1, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                        )
                );

                float aspect = float(renderFrame->GetExtent().width) / float(renderFrame->GetExtent().height);
                glm::mat4 model = glm::rotate(glm::mat4(1.0), angle, glm::vec3(0.0, 1.0, 0.0));
                glm::mat4 view = glm::lookAt(glm::vec3(0.0, 1.0, 2.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
                glm::mat4 proj = glm::perspective(1.05f, aspect, 0.1f, 20.0f);

                commandBuffer->BindPipeline(pipeline);

                // mesh 1
                {
                    glm::mat4 m = glm::translate(model, glm::vec3(0.0, 0.0, -1.0));
                    glm::mat4 mvp = proj * view * m;

                    auto descriptor = SlimPtr<Descriptor>(renderFrame, pipeline);
                    descriptor->SetUniform("Camera", renderFrame->RequestUniformBuffer(mvp));
                    descriptor->SetTexture("Albedo", texture, sampler);

                    commandBuffer->BindDescriptor(descriptor.get());
                    commandBuffer->BindVertexBuffer(0, vBuffer.get(), 0);
                    commandBuffer->BindIndexBuffer(iBuffer.get());
                    commandBuffer->DrawIndexed(6, 1, 0, 0, 0);
                }

                // mesh 2
                {
                    glm::mat4 m = glm::translate(model, glm::vec3(0.0, 0.0, -2.0));
                    glm::mat4 mvp = proj * view * m;

                    auto descriptor = SlimPtr<Descriptor>(renderFrame, pipeline);
                    descriptor->SetUniform("Camera", renderFrame->RequestUniformBuffer(mvp));
                    descriptor->SetTexture("Albedo", texture, sampler);

                    commandBuffer->BindDescriptor(descriptor.get());
                    commandBuffer->BindVertexBuffer(0, vBuffer.get(), 0);
                    commandBuffer->BindIndexBuffer(iBuffer.get());
                    commandBuffer->DrawIndexed(6, 1, 0, 0, 0);
                }
            });

            auto uiPass = graph.CreateRenderPass("ui");
            uiPass->SetColor(backbuffer);
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
    }

    context->WaitIdle();

    return EXIT_SUCCESS;
}
