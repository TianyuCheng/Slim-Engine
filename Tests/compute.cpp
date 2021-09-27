#include "common.h"

// Test compute shader
TEST(SlimCore, ComputeShader) {
    auto contextDesc = ContextDesc()
        .EnableCompute();
    auto context= SlimPtr<Context>(contextDesc);
    auto device = SlimPtr<Device>(context);
    auto data = GenerateSequence<uint32_t>(256);
    auto srcBuffer = SlimPtr<HostStorageBuffer>(device, BufferSize(data));
    auto dstBuffer = SlimPtr<HostStorageBuffer>(device, BufferSize(data));
    auto shader = SlimPtr<spirv::ComputeShader>(device, "main", "shaders/simple.comp.spv");
    auto pipeline = SlimPtr<Pipeline>(
        device,
        ComputePipelineDesc()
            .SetComputeShader(shader)
            .SetPipelineLayout(PipelineLayoutDesc()
                .AddBinding("InputBuffer",  SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                .AddBinding("OutputBuffer", SetBinding { 0, 1 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            )
    );
    device->Execute([=](auto renderFrame, auto commandBuffer) {
        // update data for source buffer
        srcBuffer->SetData(data);

        // prepare resources
        auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetStorageBuffer("InputBuffer", srcBuffer);
        descriptor->SetStorageBuffer("OutputBuffer", dstBuffer);

        // bind and dispatch
        commandBuffer->BindPipeline(pipeline);
        commandBuffer->BindDescriptor(descriptor, VK_PIPELINE_BIND_POINT_COMPUTE);
        commandBuffer->Dispatch(1, 1, 1);

    }, VK_QUEUE_COMPUTE_BIT);

    // check
    uint32_t *output = dstBuffer->GetData<uint32_t>();
    for (uint32_t i = 0; i < dstBuffer->Size<uint32_t>(); i++) {
        EXPECT_EQ(data[i] * 2 + 1, output[i]);
    }
}

int main(int argc, char **argv) {
    // prepare for slim environment
    slim::Initialize();

    // run selected tests
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
