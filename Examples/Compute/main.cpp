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

    // prepare compute pipeline
    auto shader = SlimPtr<spirv::ComputeShader>(device, "main", "shaders/simple.comp.spv");
    auto pipeline = SlimPtr<Pipeline>(
            device,
            ComputePipelineDesc()
                .SetComputeShader(shader)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("InputBuffer",  0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("OutputBuffer", 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                )
    );

    // prepare data
    std::vector<uint32_t> data;
    for (uint32_t i = 0; i < 256; i++) data.push_back(i & 0xff);

    // create a buffers
    auto srcBuffer = SlimPtr<DeviceStorageBuffer>(device, BufferSize(data));
    auto dstBuffer = SlimPtr<DeviceStorageBuffer>(device, BufferSize(data));

    // execute
    device->Execute([=](auto renderFrame, auto commandBuffer) {

        // update data for source buffer
        srcBuffer->SetData(data);

        // prepare resources
        auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetStorageBuffer("InputBuffer", srcBuffer);
        descriptor->SetStorageBuffer("OutputBuffer", dstBuffer);

        // bind compute pipeline
        commandBuffer->BindPipeline(pipeline);

        // bind pipeline resource
        commandBuffer->BindDescriptor(descriptor, VK_PIPELINE_BIND_POINT_COMPUTE);

        // dispatch compute
        commandBuffer->Dispatch(1, 1, 1);

    }, VK_QUEUE_COMPUTE_BIT);

    uint32_t *output = dstBuffer->GetData<uint32_t>();
    for (uint32_t i = 0; i < dstBuffer->Size<uint32_t>(); i++) {
        uint32_t byte = output[i];
        std::cout << "DATA[" << i << "] = " << byte << std::endl;
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
