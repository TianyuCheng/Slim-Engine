#include "common.h"

// Test rasterization
TEST(SlimCore, Rasterization) {
    auto contextDesc = ContextDesc()
        .EnableGraphics();
    auto context= SlimPtr<Context>(contextDesc);
    auto device = SlimPtr<Device>(context);
    auto vShader = SlimPtr<spirv::VertexShader>(device, "shaders/simple.vert.spv");
    auto fShader = SlimPtr<spirv::FragmentShader>(device, "shaders/simple.frag.spv");

    // auto extent = VkExtent2D { 32, 32 };
    auto extent = VkExtent2D { 2, 2 };

    // create vertex and index buffers
    auto vBuffer = GenerateQuadVertices(device);
    auto iBuffer = GenerateQuadIndices(device);
    auto texture = GenerateCheckerboard(device, extent.width, extent.height);
    auto sampler = SlimPtr<Sampler>(device, SamplerDesc {});

    // back buffer
    auto format = VK_FORMAT_R8G8B8A8_UNORM;
    auto image = SlimPtr<GPUImage>(device, format, extent, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    auto frame = SlimPtr<RenderFrame>(device, image);

    // render graph
    RenderGraph graph(frame);

    auto backBuffer = graph.CreateResource(frame->GetBackBuffer());
    auto colorPass = graph.CreateRenderPass("color");
    colorPass->SetColor(backBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    colorPass->Execute([&](const RenderInfo &info) {
        // pipeline
        auto pipeline = info.renderFrame->RequestPipeline(
            GraphicsPipelineDesc()
                .SetName("colorPass")
                .AddVertexBinding(0, sizeof(glm::vec2) + sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX, {
                    { 0, VK_FORMAT_R32G32_SFLOAT, 0,                },
                    { 1, VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec2) },
                 })
                .SetVertexShader(vShader)
                .SetFragmentShader(fShader)
                .SetViewport(frame->GetExtent())
                .SetCullMode(VK_CULL_MODE_BACK_BIT)
                .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .SetRenderPass(info.renderPass)
                .SetDepthTest(VK_COMPARE_OP_LESS)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("MainTex", SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                )
        );

        auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetTexture("MainTex", texture, sampler);

        info.commandBuffer->BindPipeline(pipeline);
        info.commandBuffer->BindDescriptor(descriptor, VK_PIPELINE_BIND_POINT_GRAPHICS);
        info.commandBuffer->BindVertexBuffer(0, vBuffer, 0);
        info.commandBuffer->BindIndexBuffer(iBuffer);
        info.commandBuffer->DrawIndexed(6, 1, 0, 0, 0);
    });

    graph.Execute();
    device->WaitIdle();

    // copy back
    auto reference = GenerateCheckerboard(extent.width, extent.height);
    auto buffer = SlimPtr<StagingBuffer>(device, extent.width * extent.height * 4);
    device->Execute([&](auto cmd) {
        VkOffset3D off = VkOffset3D { 0, 0, 0 };
        VkExtent3D ext = VkExtent3D { extent.width, extent.height, 1 };
        cmd->SaveImage("simple-textured.png", image);
        cmd->CopyImageToBuffer(image, off, ext, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT, buffer, 0, 0, 0);
    });

    // check reference
    uint32_t *data = buffer->GetData<uint32_t>();
    CompareSequence(reference.data(), data, reference.size());
}

int main(int argc, char **argv) {
    // prepare for slim environment
    slim::Initialize();

    // run selected tests
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
