#ifndef SLIM_CORE_IMAGE_H
#define SLIM_CORE_IMAGE_H

#include <list>
#include <string>
#include <vector>
#include <unordered_map>

#include "core/vulkan.h"
#include "core/hasher.h"
#include "core/device.h"
#include "utility/transient.h"
#include "utility/interface.h"

namespace slim {

    class Image;

    template <typename Image>
    class ImagePool final : public ReferenceCountable {
        friend class Transient<Image>;
        using List = std::list<Image*>;
        using Dictionary = std::unordered_map<size_t, List>;
    public:
        explicit ImagePool(Device* device);
        virtual ~ImagePool();
        void Reset();
        Transient<Image> Request(VkFormat format,
                                 VkExtent2D extent,
                                 uint32_t mipLevels,
                                 uint32_t arrayLayers,
                                 VkSampleCountFlagBits samples,
                                 VkImageUsageFlags imageUsage);
    private:
        Image* AllocateImage(VkFormat format,
                             VkExtent2D extent,
                             uint32_t mipLevels,
                             uint32_t arrayLayers,
                             VkSampleCountFlagBits samples,
                             VkImageUsageFlags imageUsage,
                             size_t hash);
        void Recycle(Image *image, size_t size);
    private:
        SmartPtr<Device> device;
        Dictionary allAllocations;
        Dictionary availableLists;
    };

    template <typename Image>
    ImagePool<Image>::ImagePool(Device* device) : device(device) {
    }

    template <typename Image>
    ImagePool<Image>::~ImagePool() {
        // delete all allocations
        for (auto &kv : allAllocations)
            for (auto &image : kv.second)
                delete image;

        allAllocations.clear();
        availableLists.clear();
    }

    template <typename Image>
    void ImagePool<Image>::Reset() {
        // NOTE: never release allocations unless the pool is destroyed.
        // Usually the usage of images inside a frame is very regular,
        // it is possible to reuse the existing images as much as possible.
        availableLists = allAllocations;
    }

    template <typename Image>
    Transient<Image> ImagePool<Image>::Request(VkFormat format,
                                                 VkExtent2D extent,
                                                 uint32_t mipLevels,
                                                 uint32_t arrayLayers,
                                                 VkSampleCountFlagBits samples,
                                                 VkImageUsageFlags imageUsage) {
        size_t hash = slim::HashCombine(0, format, extent.width, extent.height,
                                        mipLevels, arrayLayers, samples, imageUsage);

        #define ARGS format, extent, mipLevels, arrayLayers, samples, imageUsage
        // check if image of requested size exists at all
        auto it = availableLists.find(hash);
        if (it == availableLists.end()) {
            return Transient<Image>(this, AllocateImage(ARGS, hash), hash);
        }

        // check if any existing image of request sizes is available
        auto &list = it->second;
        if (list.empty()) {
            return Transient<Image>(this, AllocateImage(ARGS, hash), hash);
        }
        #undef ARGS

        // use an existing image
        Image *image = list.back();
        list.pop_back();

        // it will recycle itself on destruction
        return Transient<Image>(this, image, hash);
    }

    template <typename Image>
    Image* ImagePool<Image>::AllocateImage(VkFormat format,
                                             VkExtent2D extent,
                                             uint32_t mipLevels,
                                             uint32_t arrayLayers,
                                             VkSampleCountFlagBits samples,
                                             VkImageUsageFlags imageUsage,
                                             size_t hash) {
        Image *image = new Image(device, format, extent, mipLevels, arrayLayers, samples, imageUsage);
        auto it = allAllocations.find(hash);
        if (it == allAllocations.end()) {
            allAllocations.insert(std::make_pair(hash, List()));
            it = allAllocations.find(hash);
        }
        it->second.push_back(image);
        return image;
    }

    template <typename Image>
    void ImagePool<Image>::Recycle(Image *image, size_t size) {
        auto it = availableLists.find(size);
        if (it == availableLists.end()) {
            availableLists.insert(std::make_pair(size, List()));
            it = availableLists.find(size);
        }
        it->second.push_back(image);
    }

    // --------------------------------------------------------

    class Image : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkImage> {
        friend class CommandBuffer;
    public:
        explicit Image(Device* device,
                       VkFormat format,
                       VkExtent2D extent,
                       uint32_t mipLevels,
                       uint32_t arrayLayers,
                       VkSampleCountFlagBits samples,
                       VkImageUsageFlags imageUsage,
                       VmaMemoryUsage memoryUsage,
                       VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
                       VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);

        explicit Image(Device* device,
                       VkFormat format,
                       VkExtent3D extent,
                       uint32_t mipLevels,
                       uint32_t arrayLayers,
                       VkSampleCountFlagBits samples,
                       VkImageUsageFlags imageUsage,
                       VmaMemoryUsage memoryUsage,
                       VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
                       VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);

        explicit Image(Device* device,
                       VkFormat format,
                       VkExtent2D extent,
                       uint32_t mipLevels,
                       uint32_t arrayLayers,
                       VkSampleCountFlagBits samples,
                       VkImage image);

        explicit Image(Device* device,
                       VkFormat format,
                       VkExtent3D extent,
                       uint32_t mipLevels,
                       uint32_t arrayLayers,
                       VkSampleCountFlagBits samples,
                       VkImage image);

        virtual ~Image();

        VkExtent3D GetExtent() const { return createInfo.extent; }

        uint32_t Layers() const { return createInfo.arrayLayers; }

        uint32_t MipLevels() const { return createInfo.mipLevels; }

        uint32_t Width() const { return createInfo.extent.width; }

        uint32_t Height() const { return createInfo.extent.height; }

        uint32_t Depth() const { return createInfo.extent.depth; }

        VkFormat GetFormat() const { return createInfo.format; }

        VkSampleCountFlagBits GetSamples() const { return createInfo.samples; }

        VkImageView AsTexture() const;
        VkImageView AsColorBuffer() const;
        VkImageView AsDepthBuffer() const;
        VkImageView AsStencilBuffer() const;
        VkImageView AsDepthStencilBuffer() const;
        VkImageView AsAutomaticView() const;

        void SetName(const std::string& name) const;

        // NOTE: not intended for manual update
        std::vector<std::vector<VkImageLayout>> layouts = {};

    private:
        Device*           device = nullptr;
        VmaAllocator      allocator = VK_NULL_HANDLE;
        VmaAllocation     allocation;
        VmaAllocationInfo allocInfo;
        VkImageCreateInfo createInfo = {};

        // common view types
        mutable VkImageView textureView      = VK_NULL_HANDLE;
        mutable VkImageView colorView        = VK_NULL_HANDLE;
        mutable VkImageView depthView        = VK_NULL_HANDLE;
        mutable VkImageView stencilView      = VK_NULL_HANDLE;
        mutable VkImageView depthStencilView = VK_NULL_HANDLE;
    };

    class ImageUsageBuilder {
        VkImageUsageFlags flags = 0;
    public:
        VkImageUsageFlags Build() const { return flags; }

        void ForSample() { flags |= VK_IMAGE_USAGE_SAMPLED_BIT; }
        void ForStorage() { flags |= VK_IMAGE_USAGE_STORAGE_BIT; }
        void ForInputAttachment() { flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT; }
        void ForColorAttachment() { flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; }
        void ForDepthStencilAttachment() { flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; }
    };

    template <VmaMemoryUsage memoryUsage,
              VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
              VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE>
    class ImageBase : public Image {
    public:
        friend class ImagePool<ImageBase>;
        using PoolType = ImagePool<ImageBase>;

        ImageBase(Device* device,
                  VkFormat format, VkExtent2D extent,
                  uint32_t mipLevels, uint32_t arrayLayers,
                  VkSampleCountFlagBits samples,
                  VkImageUsageFlags imageUsage)
            : Image(device, format, extent, mipLevels, arrayLayers, samples, imageUsage, memoryUsage) {
            // do nothing
        }

        ImageBase(Device* device,
                  VkFormat format, VkExtent3D extent,
                  uint32_t mipLevels, uint32_t arrayLayers,
                  VkSampleCountFlagBits samples,
                  VkImageUsageFlags imageUsage)
            : Image(device, format, extent, mipLevels, arrayLayers, samples, imageUsage, memoryUsage) {
            // do nothing
        }

        ImageBase(Device* device,
             VkFormat format,
             VkExtent2D extent,
             uint32_t mipLevels,
             uint32_t arrayLayers,
             VkSampleCountFlagBits samples,
             VkImage image)
            : Image(device, format, extent, mipLevels, arrayLayers, samples, image) {
            // do nothing
        }

        ImageBase(Device* device,
             VkFormat format,
             VkExtent3D extent,
             uint32_t mipLevels,
             uint32_t arrayLayers,
             VkSampleCountFlagBits samples,
             VkImage image)
            : Image(device, format, extent, mipLevels, arrayLayers, samples, image) {
            // do nothing
        }
    };

    using GPUImage = ImageBase<VMA_MEMORY_USAGE_GPU_ONLY>;
    using CPUImage = ImageBase<VMA_MEMORY_USAGE_CPU_ONLY, VK_IMAGE_TILING_LINEAR>;  // A known limitation on MoltenVK, HOST_COHEERENT image memory is not supported.

} // end of namespace slim

#endif // end of SLIM_CORE_IMAGE_H
