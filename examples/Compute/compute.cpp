#include <slim/slim.hpp>

using namespace slim;

int main() {
    // create a vulkan context
    auto context = SlimPtr<Context>(
        ContextDesc()
            .EnableCompute(true)
            .EnableGraphics(true)
            .EnableValidation(true)
    );

    // prepare compute pipeline
    auto shader = SlimPtr<spirv::ComputeShader>(context.get(), "main", "shaders/compute.comp.spv");
    auto pipeline = SlimPtr<Pipeline>(
            context.get(),
            ComputePipelineDesc()
                .SetComputeShader(shader.get())
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("InputBuffer",  0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("OutputBuffer", 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                )
    );

    // prepare data
    std::vector<uint32_t> data;
    for (uint32_t i = 0; i < 256; i++) data.push_back(i & 0xff);

    // create a buffers
    auto srcBuffer = SlimPtr<DeviceStorageBuffer>(context.get(), BufferSize(data));
    auto dstBuffer = SlimPtr<DeviceStorageBuffer>(context.get(), BufferSize(data));

    // execute
    context->Execute([=](auto renderFrame, auto commandBuffer) {

        // update data for source buffer
        srcBuffer->SetData(data);

        // prepare resources
        auto descriptor = SlimPtr<Descriptor>(renderFrame, pipeline.get());
        descriptor->SetStorage("InputBuffer", srcBuffer.get());
        descriptor->SetStorage("OutputBuffer", dstBuffer.get());

        // bind compute pipeline
        commandBuffer->BindPipeline(pipeline.get());

        // bind pipeline resource
        commandBuffer->BindDescriptor(descriptor.get());

        // dispatch compute
        commandBuffer->Dispatch(1, 1, 1);

    }, VK_QUEUE_COMPUTE_BIT);

    uint32_t *output = dstBuffer->GetData<uint32_t>();
    for (uint32_t i = 0; i < dstBuffer->Size<uint32_t>(); i++) {
        uint32_t byte = output[i];
        std::cout << "DATA[" << i << "] = " << byte << std::endl;
    }

    return EXIT_SUCCESS;
}
