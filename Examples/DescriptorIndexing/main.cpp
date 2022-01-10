#include <slim/slim.hpp>

using namespace slim;

struct Vertex {
    glm::vec3 position;
    glm::vec2 texcoord;
};

struct CameraData {
    glm::mat4 proj;
    glm::mat4 view;
};

struct ModelData {
    glm::mat4 model;
};

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
            .EnableDescriptorIndexing()
    );

    // create a slim device
    auto device = SlimPtr<Device>(context);

    // create a slim window
    auto window = SlimPtr<Window>(
        device,
        WindowDesc()
            .SetResolution(640, 480)
            .SetResizable(true)
            .SetTitle("Bindless Texture")
    );

    // create vertex and index buffers
    auto vBuffer = SlimPtr<VertexBuffer>(device, 4 * sizeof(Vertex));
    auto iBuffer = SlimPtr<IndexBuffer>(device, 256);

    // create vertex and fragment shaders
    auto vShader = SlimPtr<spirv::VertexShader>(device, "shaders/main.spv");
    auto fShader = SlimPtr<spirv::FragmentShader>(device, "shaders/main.spv");

    SmartPtr<Sampler> sampler = SlimPtr<Sampler>(device, SamplerDesc());
    SmartPtr<GPUImage> texture1;
    SmartPtr<GPUImage> texture2;

    // initialize
    device->Execute([&](CommandBuffer *commandBuffer) {
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

        TextureLoader::FlipVerticallyOnLoad();
        texture1 = TextureLoader::Load2D(commandBuffer, GetUserAsset("Pictures/VulkanOpaque.png"), VK_FILTER_LINEAR);
        texture2 = TextureLoader::Load2D(commandBuffer, GetUserAsset("Pictures/VulkanTransparent.png"), VK_FILTER_LINEAR);
    });

    // window
    while (window->IsRunning()) {
        Window::PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();
        float aspect = float(frame->GetExtent().width) / float(frame->GetExtent().height);

        constexpr uint32_t T = 32;      // on MoltenVk -> sampled images=128
        // constexpr uint32_t S = 16;      // on MoltenVk -> max samplers=16
        constexpr uint32_t N = 10;      // we only draw 10 objects

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
                            { 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, position)) },
                            { 1, VK_FORMAT_R32G32_SFLOAT,    static_cast<uint32_t>(offsetof(Vertex, texcoord)) },
                         })
                        .SetVertexShader(vShader)
                        .SetFragmentShader(fShader)
                        .SetViewport(frame->GetExtent())
                        .SetCullMode(VK_CULL_MODE_BACK_BIT)
                        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
                        .SetRenderPass(info.renderPass)
                        .SetDepthTest(VK_COMPARE_OP_LESS)
                        .SetPipelineLayout(PipelineLayoutDesc()
                            .AddPushConstant("Material", Range      { 0, 4 },    VK_SHADER_STAGE_VERTEX_BIT)
                            .AddBinding     ("Camera",   SetBinding { 0, 0 },    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
                            .AddBinding     ("Sampler",  SetBinding { 0, 1 },    VK_DESCRIPTOR_TYPE_SAMPLER,        VK_SHADER_STAGE_FRAGMENT_BIT)
                            .AddBindingArray("Images",   SetBinding { 1, 0 }, T, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT) // bindless
                            .AddBinding     ("Models",   SetBinding { 2, 0 },    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
                        )
                );

                commandBuffer->BindPipeline(pipeline);

                // prepare camera uniform buffer
                auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), pipeline->Layout());
                {
                    CameraData cameraData;
                    cameraData.proj = glm::perspective(1.05f, aspect, 0.1f, 20.0f);
                    cameraData.view = glm::lookAt(glm::vec3(0.0, 1.0, 2.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
                    descriptor->SetUniformBuffer("Camera", renderFrame->RequestUniformBuffer(cameraData));
                }

                // prepare bindless textures
                // TODO: we need a way to properly define variable descriptor count when allocating descriptor set
                descriptor->SetSampledImages("Images", { texture1, texture2 });
                descriptor->SetSampler("Sampler", sampler);

                // prepare model uniform buffer
                std::vector<SmartPtr<Descriptor>> modelDescriptors(N);
                for (uint32_t i = 0; i < N; i++) {
                    auto model = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 0.0, -float(i)));
                    modelDescriptors[i] = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), pipeline->Layout());
                    modelDescriptors[i]->SetUniformBuffer("Models", renderFrame->RequestUniformBuffer(model));
                }

                // draw objects
                commandBuffer->BindVertexBuffer(0, vBuffer, 0);
                commandBuffer->BindIndexBuffer(iBuffer);
                commandBuffer->BindDescriptor(descriptor, pipeline->Type());
                for (uint32_t i = 0; i < N; i++) {
                    int material = (i % 2);
                    commandBuffer->PushConstants(pipeline->Layout(), "Material", &material);
                    commandBuffer->BindDescriptor(modelDescriptors[i], pipeline->Type());
                    commandBuffer->DrawIndexed(6, 1, 0, 0, 0);
                }

            });
        }

        graph.Execute();
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
