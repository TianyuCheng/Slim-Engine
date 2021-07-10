#include "core/debug.h"
#include "core/commands.h"
#include "core/vkutils.h"
#include "core/texture.h"

using namespace slim;

GPUImage2D* TextureLoader::Load2D(CommandBuffer *commandBuffer, const std::string &filename, VkFilter filter) {
    stbi_set_flip_vertically_on_load(true);

    GPUImage2D *image = nullptr;
    bool isHDR = stbi_is_hdr(filename.c_str());

    int width = 0;
    int height = 0;
    int channels = 0;
    int requestChannels = 4;

    if (isHDR) {
        float *dataHDR;
        dataHDR = stbi_loadf(filename.c_str(), &width, &height, &channels, requestChannels);
        image = TextureLoader::Load2DHDR(commandBuffer, dataHDR, width, height, requestChannels, filter);
        stbi_image_free(dataHDR);
    } else {
        uint8_t *dataLDR;
        dataLDR = stbi_load(filename.c_str(), &width, &height, &channels, requestChannels);
        image = TextureLoader::Load2DLDR(commandBuffer, dataLDR, width, height, requestChannels, filter);
        stbi_image_free(dataLDR);
    }

    return image;
}

GPUImage2D* TextureLoader::Load2DLDR(CommandBuffer *commandBuffer,
                                uint8_t *data, uint32_t width, uint32_t height,
                                uint32_t numChannels, VkFilter filter) {

    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    size_t size = width * height * numChannels * sizeof(uint8_t);

    VkFormat format;
    switch (numChannels) {
        case 1: format = VK_FORMAT_R8_SRGB;       break;
        case 2: format = VK_FORMAT_R8G8_SRGB;     break;
        case 3: format = VK_FORMAT_R8G8B8_SRGB;   break;
        case 4: format = VK_FORMAT_R8G8B8A8_SRGB; break;
        default: throw std::runtime_error("invalid number of channels while loading texture image");
    }

    GPUImage2D* image = new GPUImage2D(commandBuffer->GetContext(), format, VkExtent2D { width, height }, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT);
    commandBuffer->CopyDataToImage(data, size, image, {0, 0, 0}, {width, height, 1}, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    commandBuffer->GenerateMipmaps(image, filter);
    commandBuffer->PrepareForShaderRead(image);
    return image;
}

GPUImage2D* TextureLoader::Load2DHDR(CommandBuffer *commandBuffer,
                                     float *data, uint32_t width, uint32_t height,
                                     uint32_t numChannels, VkFilter filter) {

    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    size_t size = width * height * numChannels * sizeof(float);

    VkFormat format;
    switch (numChannels) {
        case 1: format = VK_FORMAT_R32_SFLOAT;          break;
        case 2: format = VK_FORMAT_R32G32_SFLOAT;       break;
        case 3: format = VK_FORMAT_R32G32B32_SFLOAT;    break;
        case 4: format = VK_FORMAT_R32G32B32A32_SFLOAT; break;
        default: throw std::runtime_error("invalid number of channels while loading texture image");
    }

    GPUImage2D* image = new GPUImage2D(commandBuffer->GetContext(), format, VkExtent2D { width, height }, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT);
    commandBuffer->CopyDataToImage(data, size, image, {0, 0, 0}, {width, height, 1}, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    commandBuffer->GenerateMipmaps(image, filter);
    commandBuffer->PrepareForShaderRead(image);
    return image;
}
