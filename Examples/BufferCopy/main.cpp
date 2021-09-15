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

    // create a buffers
    auto buffer1 = SlimPtr<StagingBuffer>(device, 256);
    auto buffer2 = SlimPtr<VertexBuffer>(device, 256);
    auto buffer3 = SlimPtr<StagingBuffer>(device, 256);

    // copy
    device->Execute([=](auto commandBuffer) {

        // prepare data
        std::vector<uint8_t> data;
        for (uint32_t i = 0; i < 256; i++) {
            data.push_back(i & 0xff);
        }

        // update data for buffer 1
        buffer1->SetData(data);

        // copy from buffer 1 to buffer 2
        commandBuffer->CopyBufferToBuffer(buffer1, 0, buffer2, 0, 256);

        // copy from buffer 2 to buffer 3
        commandBuffer->CopyBufferToBuffer(buffer2, 0, buffer3, 0, 256);

    }, VK_QUEUE_TRANSFER_BIT);

    // output
    uint8_t *output = buffer3->GetData<uint8_t>();
    for (uint32_t i = 0; i < buffer3->Size<uint8_t>(); i++) {
        uint32_t byte = output[i];
        std::cout << "DATA[" << i << "] = " << byte << std::endl;
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
