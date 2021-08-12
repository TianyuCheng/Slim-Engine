#include "core/debug.h"
#include "core/commands.h"
#include "core/vkutils.h"
#include "utility/texture.h"

using namespace slim;

void TextureLoader::FlipVerticallyOnLoad(bool value) {
    stbi_set_flip_vertically_on_load(value);
}

GPUImage* TextureLoader::Load2D(CommandBuffer *commandBuffer, const std::string &filename, VkFilter filter) {
    GPUImage *image = nullptr;
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

GPUImage* TextureLoader::Load2DLDR(CommandBuffer *commandBuffer,
                                uint8_t *data, uint32_t width, uint32_t height,
                                uint32_t numChannels, VkFilter filter) {

    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    size_t size = width * height * numChannels * sizeof(uint8_t);
    uint32_t arrayLayers = 1;

    VkFormat format;
    switch (numChannels) {
        case 1: format = VK_FORMAT_R8_SRGB;       break;
        case 2: format = VK_FORMAT_R8G8_SRGB;     break;
        case 3: format = VK_FORMAT_R8G8B8_SRGB;   break;
        case 4: format = VK_FORMAT_R8G8B8A8_SRGB; break;
        default: throw std::runtime_error("invalid number of channels while loading texture image");
    }

    GPUImage* image = new GPUImage(commandBuffer->GetDevice(), format, VkExtent2D { width, height }, mipLevels, arrayLayers, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT);
    commandBuffer->CopyDataToImage(data, size, image, {0, 0, 0}, {width, height, 1}, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    commandBuffer->GenerateMipmaps(image, filter);
    commandBuffer->PrepareForShaderRead(image);
    return image;
}

GPUImage* TextureLoader::Load2DHDR(CommandBuffer *commandBuffer,
                                     float *data, uint32_t width, uint32_t height,
                                     uint32_t numChannels, VkFilter filter) {

    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    size_t size = width * height * numChannels * sizeof(float);
    uint32_t arrayLayers = 1;

    VkFormat format;
    switch (numChannels) {
        case 1: format = VK_FORMAT_R32_SFLOAT;          break;
        case 2: format = VK_FORMAT_R32G32_SFLOAT;       break;
        case 3: format = VK_FORMAT_R32G32B32_SFLOAT;    break;
        case 4: format = VK_FORMAT_R32G32B32A32_SFLOAT; break;
        default: throw std::runtime_error("invalid number of channels while loading texture image");
    }

    GPUImage* image = new GPUImage(commandBuffer->GetDevice(), format, VkExtent2D { width, height }, mipLevels, arrayLayers, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT);
    commandBuffer->CopyDataToImage(data, size, image, {0, 0, 0}, {width, height, 1}, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    commandBuffer->GenerateMipmaps(image, filter);
    commandBuffer->PrepareForShaderRead(image);
    return image;
}

GPUImage* TextureLoader::LoadCubemap(CommandBuffer* commandBuffer,
                                     const std::string& xpos,
                                     const std::string& xneg,
                                     const std::string& ypos,
                                     const std::string& yneg,
                                     const std::string& zpos,
                                     const std::string& zneg,
                                     VkFilter filter) {

    // assume all 6 images have the same dimension

    GPUImage *image = nullptr;
    bool isHDR = stbi_is_hdr(xpos.c_str());

    if (isHDR) {
        image = TextureLoader::LoadCubemapHDR(commandBuffer, xpos, xneg, ypos, yneg, zpos, zneg, filter);
    } else {
        image = TextureLoader::LoadCubemapHDR(commandBuffer, xpos, xneg, ypos, yneg, zpos, zneg, filter);
    }

    return image;
}

GPUImage* TextureLoader::LoadCubemapLDR(CommandBuffer* commandBuffer,
                                        const std::string& xpos,
                                        const std::string& xneg,
                                        const std::string& ypos,
                                        const std::string& yneg,
                                        const std::string& zpos,
                                        const std::string& zneg,
                                        VkFilter filter) {
    std::array<std::string, 6> images = {
        xpos, xneg,
        ypos, yneg,
        zpos, zneg,
    };

    int width = 0;
    int height = 0;
    int channels = 0;
    int numChannels = 4;

    uint8_t *data;
    data = stbi_load(images[0].c_str(), &width, &height, &channels, numChannels);

    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    size_t size = width * height * numChannels * sizeof(uint8_t);
    uint32_t arrayLayers = 6;

    VkFormat format;
    switch (numChannels) {
        case 1: format = VK_FORMAT_R8_SRGB;       break;
        case 2: format = VK_FORMAT_R8G8_SRGB;     break;
        case 3: format = VK_FORMAT_R8G8B8_SRGB;   break;
        case 4: format = VK_FORMAT_R8G8B8A8_SRGB; break;
        default: throw std::runtime_error("invalid number of channels while loading texture image");
    }

    VkOffset3D offset1 = { 0, 0, 0 };
    VkExtent3D offset2 = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

    GPUImage* image = new GPUImage(commandBuffer->GetDevice(), format, extent, mipLevels, arrayLayers, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT);
    commandBuffer->CopyDataToImage(data, size, image, offset1, offset2, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    stbi_image_free(data);

    for (uint32_t i = 1; i < 6; i++) {
        const std::string& filename = images[i];
        data = stbi_load(filename.c_str(), &width, &height, &channels, numChannels);
        commandBuffer->CopyDataToImage(data, size, image, offset1, offset2, i, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
        stbi_image_free(data);
    }

    commandBuffer->GenerateMipmaps(image, filter);
    commandBuffer->PrepareForShaderRead(image);
    return image;
}

GPUImage* TextureLoader::LoadCubemapHDR(CommandBuffer* commandBuffer,
                                        const std::string& xpos,
                                        const std::string& xneg,
                                        const std::string& ypos,
                                        const std::string& yneg,
                                        const std::string& zpos,
                                        const std::string& zneg,
                                        VkFilter filter) {
    std::array<std::string, 6> images = {
        xpos, xneg,
        ypos, yneg,
        zpos, zneg,
    };

    int width = 0;
    int height = 0;
    int channels = 0;
    int numChannels = 4;

    float *data;
    data = stbi_loadf(images[0].c_str(), &width, &height, &channels, numChannels);

    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    size_t size = width * height * numChannels * sizeof(float);
    uint32_t arrayLayers = 6;

    VkFormat format;
    switch (numChannels) {
        case 1: format = VK_FORMAT_R32_SFLOAT;          break;
        case 2: format = VK_FORMAT_R32G32_SFLOAT;       break;
        case 3: format = VK_FORMAT_R32G32B32_SFLOAT;    break;
        case 4: format = VK_FORMAT_R32G32B32A32_SFLOAT; break;
        default: throw std::runtime_error("invalid number of channels while loading texture image");
    }

    VkOffset3D offset1 = { 0, 0, 0 };
    VkExtent3D offset2 = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

    GPUImage* image = new GPUImage(commandBuffer->GetDevice(), format, extent, mipLevels, arrayLayers, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT);
    commandBuffer->CopyDataToImage(data, size, image, offset1, offset2, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    stbi_image_free(data);

    for (uint32_t i = 1; i < 6; i++) {
        const std::string& filename = images[i];
        data = stbi_loadf(filename.c_str(), &width, &height, &channels, numChannels);
        commandBuffer->CopyDataToImage(data, size, image, offset1, offset2, i, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
        stbi_image_free(data);
    }

    commandBuffer->GenerateMipmaps(image, filter);
    commandBuffer->PrepareForShaderRead(image);
    return image;
}
