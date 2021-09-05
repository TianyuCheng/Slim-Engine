#include <slim/slim.hpp>

using namespace slim;

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
};

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
            .SetTitle("Depth Buffering")
    );

    // create vertex and index buffers
    auto vBuffer = SlimPtr<VertexBuffer>(device, 4 * sizeof(Vertex));
    auto iBuffer = SlimPtr<IndexBuffer>(device, 256);

    // create vertex and fragment shaders
    auto vShader = SlimPtr<spirv::VertexShader>(device, "main", "shaders/simple.vert.spv");
    auto fShader = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/simple.frag.spv");

    // initialize
    device->Execute([=](CommandBuffer *commandBuffer) {
        // prepare vertex data
        std::vector<Vertex> positions = {
            { glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(1.0f, 0.0f, 0.0f) },
            { glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(0.0f, 1.0f, 0.0f) },
            { glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(1.0f, 1.0f, 0.0f) },
            { glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f) },
        };
        std::vector<uint32_t> indices = {
            0, 1, 2,
            2, 3, 0,
        };

        commandBuffer->CopyDataToBuffer(positions, vBuffer);
        commandBuffer->CopyDataToBuffer(indices, iBuffer);
    });

    // window
    while (window->IsRunning()) {
        Window::PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();
        float aspect = float(frame->GetExtent().width) / float(frame->GetExtent().height);

        // rendergraph-based design
        RenderGraph graph(frame);
        {
            auto backBuffer = graph.CreateResource(frame->GetBackBuffer());
            auto depthBuffer = graph.CreateResource(frame->GetExtent(), VK_FORMAT_D24_UNORM_S8_UINT, VK_SAMPLE_COUNT_1_BIT);

            auto colorPass = graph.CreateRenderPass("color");
            colorPass->SetColor(backBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            colorPass->SetDepthStencil(depthBuffer, ClearValue(1.0f, 0));
            colorPass->Execute([=](const RenderInfo &info) {
                auto renderFrame = info.renderFrame;
                auto commandBuffer = info.commandBuffer;
                auto pipeline = renderFrame->RequestPipeline(
                    GraphicsPipelineDesc()
                        .SetName("colorPass")
                        .AddVertexBinding(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
                            { 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) },
                            { 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)    },
                         })
                        .SetVertexShader(vShader)
                        .SetFragmentShader(fShader)
                        .SetViewport(frame->GetExtent())
                        .SetCullMode(VK_CULL_MODE_BACK_BIT)
                        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
                        .SetRenderPass(info.renderPass)
                        .SetDepthTest(VK_COMPARE_OP_LESS)
                        .SetPipelineLayout(PipelineLayoutDesc()
                            .AddBinding("Camera", 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
                        )
                );

                glm::mat4 model = glm::mat4(1.0);
                glm::mat4 view = glm::lookAt(glm::vec3(0.0, 1.0, 2.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
                glm::mat4 proj = glm::perspective(1.05f, aspect, 0.1f, 20.0f);

                commandBuffer->BindPipeline(pipeline);

                // mesh 1
                {
                    glm::mat4 m = glm::translate(model, glm::vec3(0.0, 0.0, 0.0));
                    glm::mat4 mvp = proj * view * m;
                    auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), pipeline->Layout());
                    descriptor->SetUniformBuffer("Camera", renderFrame->RequestUniformBuffer(mvp));
                    commandBuffer->BindDescriptor(descriptor, pipeline->Type());
                    commandBuffer->BindVertexBuffer(0, vBuffer, 0);
                    commandBuffer->BindIndexBuffer(iBuffer);
                    commandBuffer->DrawIndexed(6, 1, 0, 0, 0);
                }

                // mesh 2
                {
                    glm::mat4 m = glm::translate(model, glm::vec3(0.0, 0.0, -1.0));
                    glm::mat4 mvp = proj * view * m;
                    auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), pipeline->Layout());
                    descriptor->SetUniformBuffer("Camera", renderFrame->RequestUniformBuffer(mvp));
                    commandBuffer->BindDescriptor(descriptor, pipeline->Type());
                    commandBuffer->BindVertexBuffer(0, vBuffer, 0);
                    commandBuffer->BindIndexBuffer(iBuffer);
                    commandBuffer->DrawIndexed(6, 1, 0, 0, 0);
                }
            });
        }

        graph.Execute();
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
