#include "common.h"

std::vector<uint32_t> GenerateCheckerboard(size_t width, size_t height) {
    std::vector<uint32_t> data(width * height);
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            data[y * width + x] = (x + y) % 2 == 0 ? 0xffffffff : 0x000000ff;
        }
    }
    return data;
}

SmartPtr<GPUImage> GenerateCheckerboard(Device* device, size_t width, size_t height) {
    auto data = GenerateCheckerboard(width, width);
    auto extent = VkExtent2D { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    auto format = VK_FORMAT_R8G8B8A8_UNORM;
    auto img = SlimPtr<GPUImage>(device, format, extent, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT);
    device->Execute([&](auto cmd) {
        VkOffset3D off = VkOffset3D { 0, 0, 0 };
        VkExtent3D ext = VkExtent3D { extent.width, extent.height, 1 };
        cmd->CopyDataToImage(data, img, off, ext, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    });
    return img;
}

SmartPtr<VertexBuffer> GenerateQuadVertices(Device* device) {
    struct Vertex {
        glm::vec2 position;
        glm::vec2 uv;
    };

    // create vertex buffers
    auto vBuffer = SlimPtr<VertexBuffer>(device, 4 * sizeof(Vertex));
    device->Execute([=](CommandBuffer *commandBuffer) {
        std::vector<Vertex> positions = {
            { glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec2(0.0f, 0.0f) },
            { glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec2(1.0f, 0.0f) },
            { glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec2(1.0f, 1.0f) },
            { glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec2(0.0f, 1.0f) },
        };
        commandBuffer->CopyDataToBuffer(positions, vBuffer);
    });

    return vBuffer;
}

SmartPtr<IndexBuffer> GenerateQuadIndices(Device* device) {
    // create index buffers
    auto iBuffer = SlimPtr<IndexBuffer>(device, 6 * sizeof(uint32_t));
    device->Execute([=](CommandBuffer *commandBuffer) {
        std::vector<uint32_t> indices = {
            0, 1, 2,
            2, 3, 0,
        };
        commandBuffer->CopyDataToBuffer(indices, iBuffer);
    });

    return iBuffer;
}
