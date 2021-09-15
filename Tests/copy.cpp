#include "common.h"

// Test buffer copy with slim command buffer
TEST(SlimCore, BufferCopy) {
    auto contextDesc = ContextDesc()
        .EnableValidation();
    auto context= SlimPtr<Context>(contextDesc);
    auto device = SlimPtr<Device>(context);
    auto buffer1 = SlimPtr<Buffer>(device, 256, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    auto buffer2 = SlimPtr<Buffer>(device, 256, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    auto data1 = GenerateSequence<uint8_t>(256);
    device->Execute([&](auto cmd) {
        cmd->CopyDataToBuffer(data1, buffer1);
        cmd->CopyBufferToBuffer(buffer1, 0, buffer2, 0, 256);
    });
    uint8_t *data2 = buffer2->GetData<uint8_t>();
    CompareSequence(data1.data(), data2, 256);
}

// Test image copy with slim command buffer
TEST(SlimCore, ImageBufferCopy) {
    auto contextDesc = ContextDesc()
        .EnableValidation();
    auto data1 = GenerateCheckerboard(32, 32);
    auto context= SlimPtr<Context>(contextDesc);
    auto device = SlimPtr<Device>(context);
    auto extent = VkExtent2D { 32, 32 };
    auto format = VK_FORMAT_R8G8B8A8_UNORM;
    auto img = SlimPtr<GPUImage>(device, format, extent, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_STORAGE_BIT);
    auto buf = SlimPtr<StagingBuffer>(device, BufferSize(data1));
    device->Execute([&](auto cmd) {
        VkOffset3D off = VkOffset3D { 0, 0, 0 };
        VkExtent3D ext = VkExtent3D { extent.width, extent.height, 1 };
        cmd->CopyDataToImage(data1, img, off, ext, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
        cmd->CopyImageToBuffer(img, off, ext, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT, buf, 0, 0, 0);
    });

    // check
    uint32_t *data2 = buf->GetData<uint32_t>();
    CompareSequence(data1.data(), data2, data1.size());

    // save image
    device->Execute([&](auto cmd) {
        cmd->SaveImage("image-buffer-copy.png", img);
    });
}


int main(int argc, char **argv) {
    // prepare for slim environment
    slim::Initialize();

    // run selected tests
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
