#include "core/debug.h"
#include "core/image.h"
#include "core/renderpass.h"
#include "core/vkutils.h"

using namespace slim;

VkImageViewType InferImageViewType(const VkImageCreateInfo &createInfo) {
    if (createInfo.imageType == VK_IMAGE_TYPE_1D) {
        return createInfo.arrayLayers == 1
                    ? VK_IMAGE_VIEW_TYPE_1D
                    : VK_IMAGE_VIEW_TYPE_1D_ARRAY;
    }
    else if (createInfo.imageType == VK_IMAGE_TYPE_2D) {
        if (createInfo.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) {
            return createInfo.arrayLayers == 6
                        ? VK_IMAGE_VIEW_TYPE_CUBE
                        : VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        } else {
            return createInfo.arrayLayers == 1
                        ? VK_IMAGE_VIEW_TYPE_2D
                        : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        }
    }
    else if (createInfo.imageType == VK_IMAGE_TYPE_3D) {
        return VK_IMAGE_VIEW_TYPE_3D;
    }

    throw std::runtime_error("Failed to infer image view type!");
    return VK_IMAGE_VIEW_TYPE_2D;
}

Image::Image(Device* device,
             VkFormat format,
             VkExtent2D extent,
             uint32_t mipLevels,
             uint32_t arrayLayers,
             VkSampleCountFlagBits samples,
             VkImageUsageFlags imageUsage,
             VmaMemoryUsage memoryUsage,
             VkImageTiling tiling,
             VkSharingMode sharingMode)
    : Image(device, format,
            VkExtent3D { extent.width, extent.height, 1 },
            mipLevels, arrayLayers, samples,
            imageUsage, memoryUsage, tiling, sharingMode) {

}

Image::Image(Device* device,
             VkFormat format,
             VkExtent3D extent,
             uint32_t mipLevels,
             uint32_t arrayLayers,
             VkSampleCountFlagBits samples,
             VkImageUsageFlags imageUsage,
             VmaMemoryUsage memoryUsage,
             VkImageTiling tiling,
             VkSharingMode sharingMode)
    : device(device), allocator(device->GetMemoryAllocator()) {

    VkImageType imageType = VK_IMAGE_TYPE_1D;
    if (extent.height > 1) imageType = VK_IMAGE_TYPE_2D;
    if (extent.depth  > 1) imageType = VK_IMAGE_TYPE_3D;

    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.imageType = imageType;
    createInfo.format = format;
    createInfo.extent = extent;
    createInfo.mipLevels = mipLevels;
    createInfo.arrayLayers = arrayLayers;
    createInfo.samples = samples;
    createInfo.tiling = tiling;
    createInfo.usage = imageUsage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.sharingMode = sharingMode;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.flags = 0;

    if (imageType == VK_IMAGE_TYPE_2D && arrayLayers == 6) {
        createInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = memoryUsage;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    ErrorCheck(vmaCreateImage(allocator, &createInfo, &allocCreateInfo, &handle, &allocation, &allocInfo), "create image");

    // initialize image layout for each slice
    layouts.resize(arrayLayers);
    for (auto &layout : layouts) {
        for (uint32_t i = 0; i < mipLevels; i++) {
            layout.push_back(VK_IMAGE_LAYOUT_UNDEFINED);
        }
    }
}

Image::Image(Device* device,
             VkFormat format,
             VkExtent2D extent,
             uint32_t mipLevels,
             uint32_t arrayLayers,
             VkSampleCountFlagBits samples,
             VkImage image)
    : Image(device, format,
            VkExtent3D { extent.width, extent.height, 1 },
            mipLevels, arrayLayers, samples, image) {
}

Image::Image(Device* device,
             VkFormat format,
             VkExtent3D extent,
             uint32_t mipLevels,
             uint32_t arrayLayers,
             VkSampleCountFlagBits samples,
             VkImage image) : device(device) {

    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.format = format;
    createInfo.extent = extent;
    createInfo.mipLevels = mipLevels;
    createInfo.arrayLayers = arrayLayers;
    createInfo.samples = samples;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    handle = image;

    // initialize image layout for each slice
    layouts.resize(arrayLayers);
    for (auto &layout : layouts)
        for (uint32_t i = 0; i < mipLevels; i++)
            layout.push_back(VK_IMAGE_LAYOUT_UNDEFINED);
}

Image::~Image() {
    #define DESTROY_VIEW(view)                                      \
    if (view) {                                                     \
        vkDestroyImageView(*device, view, nullptr);                 \
        textureView = VK_NULL_HANDLE;                               \
    }
    DESTROY_VIEW(textureView);
    DESTROY_VIEW(colorView);
    DESTROY_VIEW(depthView);
    DESTROY_VIEW(stencilView);
    DESTROY_VIEW(depthStencilView);
    #undef DESTROY_VIEW

    if (allocator) {
        vmaDestroyImage(allocator, handle, allocation);
        allocator = VK_NULL_HANDLE;
    }
    handle = VK_NULL_HANDLE;
}

void Image::SetName(const std::string& name) const {
    if (device->IsDebuggerEnabled()) {
        VkDebugMarkerObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT;
        nameInfo.object = (uint64_t) handle;
        nameInfo.pObjectName = name.c_str();
        ErrorCheck(DeviceDispatch(vkDebugMarkerSetObjectNameEXT(*device, &nameInfo)), "set image name");
    }
}

VkImageView Image::AsTexture() const {
    if (!textureView) {
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = handle;
        viewCreateInfo.format = createInfo.format;
        viewCreateInfo.viewType = InferImageViewType(createInfo);
        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = createInfo.arrayLayers;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = createInfo.mipLevels;
        ErrorCheck(DeviceDispatch(vkCreateImageView(*device, &viewCreateInfo, nullptr, &textureView)), "create texture view");
    }
    return textureView;
}

VkImageView Image::AsColorBuffer() const {
    if (!colorView) {
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = handle;
        viewCreateInfo.format = createInfo.format;
        viewCreateInfo.viewType = InferImageViewType(createInfo);
        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = createInfo.arrayLayers;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = createInfo.mipLevels;
        ErrorCheck(DeviceDispatch(vkCreateImageView(*device, &viewCreateInfo, nullptr, &colorView)), "create color buffer view");
    }
    return colorView;
}

VkImageView Image::AsDepthBuffer() const {
    if (!depthView) {
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = handle;
        viewCreateInfo.format = createInfo.format;
        viewCreateInfo.viewType = InferImageViewType(createInfo);
        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = createInfo.arrayLayers;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = createInfo.mipLevels;
        ErrorCheck(DeviceDispatch(vkCreateImageView(*device, &viewCreateInfo, nullptr, &depthView)), "create depth buffer view");
    }
    return depthView;
}

VkImageView Image::AsStencilBuffer() const {
    if (!stencilView) {
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = handle;
        viewCreateInfo.format = createInfo.format;
        viewCreateInfo.viewType = InferImageViewType(createInfo);
        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = createInfo.arrayLayers;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = createInfo.mipLevels;
        ErrorCheck(DeviceDispatch(vkCreateImageView(*device, &viewCreateInfo, nullptr, &stencilView)), "create stencil buffer view");
    }
    return stencilView;
}

VkImageView Image::AsDepthStencilBuffer() const {
    if (!depthStencilView) {
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = handle;
        viewCreateInfo.format = createInfo.format;
        viewCreateInfo.viewType = InferImageViewType(createInfo);
        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = createInfo.arrayLayers;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = createInfo.mipLevels;
        ErrorCheck(DeviceDispatch(vkCreateImageView(*device, &viewCreateInfo, nullptr, &depthStencilView)), "create depth stencil view");
    }
    return depthStencilView;
}

VkImageView Image::AsAutomaticView() const {
    bool isDepthOnly = IsDepthOnly(GetFormat());
    bool isDepthStencil = IsDepthOnly(GetFormat());

    if (isDepthOnly) {
        return AsDepthBuffer();
    } else if (isDepthStencil) {
        return AsDepthStencilBuffer();
    } else {
        return AsTexture();
    }
}
