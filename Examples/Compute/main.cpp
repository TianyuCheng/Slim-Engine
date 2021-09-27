#include <slim/slim.hpp>

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
                    .AddBinding("InputBuffer",  SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("OutputBuffer", SetBinding { 0, 1 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("AtomicBuffer", SetBinding { 0, 2 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                )
    );

    // prepare data
    std::vector<uint32_t> data1;
    for (uint32_t i = 0; i < 256; i++) {
        data1.push_back(i & 0xff);
    }

    std::vector<uint32_t> data2;
    data2.push_back(0);

    // create buffers
    auto buffer1 = SlimPtr<Buffer>(device, BufferSize(data1), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    auto buffer2 = SlimPtr<Buffer>(device, BufferSize(data1), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);
    auto buffer3 = SlimPtr<Buffer>(device, BufferSize(data2), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    auto buffer4 = SlimPtr<Buffer>(device, BufferSize(data2), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    // execute
    device->Execute([=](auto renderFrame, auto commandBuffer) {

        // update data for source buffer
        commandBuffer->CopyDataToBuffer(data1, buffer1);
        commandBuffer->CopyDataToBuffer(data2, buffer3);

        // prepare resources
        auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetStorageBuffer("InputBuffer", buffer1);
        descriptor->SetStorageBuffer("OutputBuffer", buffer2);
        descriptor->SetStorageBuffer("AtomicBuffer", buffer3);

        // bind compute pipeline
        commandBuffer->BindPipeline(pipeline);

        // bind pipeline resource
        commandBuffer->BindDescriptor(descriptor, VK_PIPELINE_BIND_POINT_COMPUTE);

        // dispatch compute
        commandBuffer->Dispatch(1, 1, 1);

        // copy atomic data back
        commandBuffer->CopyBufferToBuffer(buffer3, 0, buffer4, 0, sizeof(uint32_t));

    }, VK_QUEUE_COMPUTE_BIT);

    uint32_t *output = buffer2->GetData<uint32_t>();
    for (uint32_t i = 0; i < buffer2->Size<uint32_t>(); i++) {
        uint32_t byte = output[i];
        std::cout << "DATA[" << i << "] = " << byte << std::endl;
    }

    uint32_t atomicResult = buffer4->GetData<uint32_t>()[0];
    std::cout << "atomic result: " << atomicResult << std::endl;

    device->WaitIdle();
    return EXIT_SUCCESS;
}
