#include <slim/slim.hpp>

using namespace slim;

struct Vertex {
    glm::vec3 position;
    glm::vec2 texcoord;
};

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
            .SetTitle("Multisampling")
    );

    // create vertex and index buffers
    auto vBuffer = SlimPtr<VertexBuffer>(context, 4 * sizeof(Vertex));
    auto iBuffer = SlimPtr<IndexBuffer>(context, 256);

    // create vertex and fragment shaders
    auto vShader = SlimPtr<spirv::VertexShader>(context, "main", "shaders/textured.vert.spv");
    auto fShader = SlimPtr<spirv::FragmentShader>(context, "main", "shaders/textured.frag.spv");

    // initialize
    context->Execute([=](CommandBuffer *commandBuffer) {
        // prepare vertex data
        std::vector<Vertex> positions = {
            { glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec2(0.0f, 0.0f) },
            { glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec2(1.0f, 0.0f) },
            { glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec2(1.0f, 1.0f) },
            { glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec2(0.0f, 1.0f) },
        };
        std::vector<uint32_t> indices = {
            0, 1, 2,
            2, 3, 0,
        };

        commandBuffer->CopyDataToBuffer(positions, vBuffer);
        commandBuffer->CopyDataToBuffer(indices, iBuffer);
    });

    // create texture
    auto renderFrame = SlimPtr<RenderFrame>(context);
    auto commandBuffer = renderFrame->RequestCommandBuffer(VK_QUEUE_TRANSFER_BIT);
    commandBuffer->Begin();
    SmartPtr<Sampler> sampler = SlimPtr<Sampler>(context, SamplerDesc());
    SmartPtr<GPUImage2D> texture = TextureLoader::Load2D(commandBuffer, ToAssetPath("Pictures/VulkanOpaque.png"), VK_FILTER_LINEAR);
    commandBuffer->End();
    commandBuffer->Submit();
    context->WaitIdle();

    // window
    auto window = context->GetWindow();
    while (!window->ShouldClose()) {
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
            colorPass->Execute([=](const RenderGraph &graph) {
                auto renderFrame = graph.GetRenderFrame();
                auto commandBuffer = graph.GetGraphicsCommandBuffer();
                auto pipeline = renderFrame->RequestPipeline(
                    GraphicsPipelineDesc()
                        .SetName("colorPass")
                        .AddVertexBinding(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
                            { 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) },
                            { 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texcoord)    },
                         })
                        .SetVertexShader(vShader)
                        .SetFragmentShader(fShader)
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

                glm::mat4 model = glm::mat4(1.0);
                glm::mat4 view = glm::lookAt(glm::vec3(0.0, 0.0, 2.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
                glm::mat4 proj = glm::perspective(1.05f, aspect, 0.1f, 20.0f);

                commandBuffer->BindPipeline(pipeline);

                auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), pipeline->Layout());
                descriptor->SetTexture("Albedo", texture, sampler);
                commandBuffer->BindDescriptor(descriptor);

                // mesh 1
                {
                    glm::mat4 m = glm::translate(model, glm::vec3(0.0, 0.0, 0.0));
                    glm::mat4 mvp = proj * view * m;
                    auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), pipeline->Layout());
                    descriptor->SetUniform("Camera", renderFrame->RequestUniformBuffer(mvp));
                    commandBuffer->BindDescriptor(descriptor);
                    commandBuffer->BindVertexBuffer(0, vBuffer, 0);
                    commandBuffer->BindIndexBuffer(iBuffer);
                    commandBuffer->DrawIndexed(6, 1, 0, 0, 0);
                }
            });
        }

        graph.Execute();

        // window update
        window->PollEvents();
    }

    context->WaitIdle();
    return EXIT_SUCCESS;
}
