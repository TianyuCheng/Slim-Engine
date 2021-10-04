#ifndef SLIM_CORE_COMMANDS_H
#define SLIM_CORE_COMMANDS_H

#include <string>
#include <vector>

#include "core/vulkan.h"
#include "core/buffer.h"
#include "core/device.h"
#include "core/vkutils.h"
#include "core/pipeline.h"
#include "core/descriptor.h"
#include "core/acceleration.h"
#include "core/synchronization.h"
#include "utility/interface.h"

namespace slim {

    class CommandPool;
    class CommandBuffer;

    class CommandBuffer final : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkCommandBuffer> {
        friend class CommandPool;
    public:
        using PoolType = CommandPool;
        explicit CommandBuffer(Device *device, VkQueue queue, VkCommandBuffer commandBuffer);
        virtual ~CommandBuffer();

        void SetName(const std::string& name) const;
        void BeginRegion(const std::string& name) const;
        void EndRegion() const;
        void InsertMarker(const std::string& marker) const;

        void Begin();
        void End();
        void Submit();
        void Wait(Semaphore *semaphore, VkPipelineStageFlags stages);
        void Signal(Semaphore *semaphore);
        void Signal(Fence *fence);

        void NextSubpass(VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);

        void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance);

        void BindPipeline(Pipeline *pipeline);
        void BindDescriptor(Descriptor *descriptor, VkPipelineBindPoint bindPoint);
        void BindDescriptor(Descriptor *descriptor, const std::vector<uint32_t> &dynamicOffset, VkPipelineBindPoint bindPoint);
        void BindIndexBuffer(IndexBuffer *buffer, size_t offset = 0);
        void BindIndexBuffer(Buffer *buffer, size_t offset, VkIndexType indexType);
        void BindVertexBuffer(uint32_t binding, Buffer *buffer, uint64_t offset);
        void BindVertexBuffers(uint32_t binding, const std::vector<Buffer*> &buffers, const std::vector<uint64_t> &offsets);

        void PushConstants(PipelineLayout *layout, const std::string &name, const void *value);
        void PushConstants(PipelineLayout *layout, size_t offset, const void *value, size_t size, VkShaderStageFlags stages);

        void CopyDataToBuffer(void *data, size_t size, Buffer *buffer, size_t offset = 0);
        void CopyDataToImage(void *data, size_t size, Image *image,
                             const VkOffset3D &offset, const VkExtent3D &extent,
                             uint32_t baseLayer, uint32_t layerCount, uint32_t mipLevel,
                             VkImageAspectFlags aspectMask);

        template <typename T>
        void CopyDataToBuffer(const T &data, Buffer *buffer, size_t offset = 0);

        template <typename T>
        void CopyDataToBuffer(const std::vector<T> &data, Buffer *buffer, size_t offset = 0);

        template <typename T>
        void CopyDataToImage(const std::vector<T> &data, Image *image,
                             const VkOffset3D &offset, const VkExtent3D &extent,
                             uint32_t baseLayer, uint32_t layerCount, uint32_t mipLevel,
                             VkImageAspectFlags aspectMask);

        void CopyBufferToBuffer(Buffer *srcBuffer, size_t srcOffset,
                                Buffer *dstBuffer, size_t dstOffset,
                                size_t size);
        void CopyBufferToImage(Buffer *srcBuffer, size_t bufferOffset, size_t bufferRowLength, size_t bufferImageHeight,
                               Image *dstImage, const VkOffset3D &offset, const VkExtent3D &extent,
                               uint32_t baseLayer, uint32_t layerCount, uint32_t mipLevel, VkImageAspectFlags aspectMask);
        void CopyImageToBuffer(Image *srcImage, const VkOffset3D &offset, const VkExtent3D &extent,
                               uint32_t baseLayer, uint32_t layerCount, uint32_t mipLevel, VkImageAspectFlags aspectMask,
                               Buffer *dstBuffer, size_t bufferOffset, size_t bufferRowLength, size_t bufferImageHeight);
        void CopyImageToImage(Image *srcImage, const VkOffset3D &srcOffset,
                              uint32_t srcBaseLayer, uint32_t srcLayerCount,
                              uint32_t srcMipLevel, VkImageAspectFlags srcAspectMask,
                              Image *dstImage, const VkOffset3D &dstOffset,
                              uint32_t dstBaseLayer, uint32_t dstLayerCount,
                              uint32_t dstMipLevel, VkImageAspectFlags dstAspectMask);
        void BlitImage(Image *srcImage, const VkOffset3D &srcOffset1, const VkOffset3D &srcOffset2,
                       uint32_t srcBaseLayer, uint32_t srcLayerCount, uint32_t srcMipLevel, VkImageAspectFlags srcAspectMask,
                       Image *dstImage, const VkOffset3D &dstOffset1, const VkOffset3D &dstOffset2,
                       uint32_t dstBaseLayer, uint32_t dstLayerCount, uint32_t dstMipLevel, VkImageAspectFlags dstAspectMask,
                       VkFilter filter);

        void ClearColor(Image* image,
                        const VkClearColorValue& clear,
                        uint32_t baseLayer = 0, uint32_t layerCount = 0,
                        uint32_t mipLevel = 0, uint32_t levelCount = 0);

        void ClearDepthStencil(Image* image,
                        const VkClearDepthStencilValue& clear,
                        uint32_t baseLayer = 0, uint32_t layerCount = 0,
                        uint32_t mipLevel = 0, uint32_t levelCount = 0);

        void SaveImage(const std::string& name, Image* image, uint32_t arrayLayer = 0, uint32_t mipLevel = 0);

        void GenerateMipmaps(Image *image, VkFilter filter);
        void PrepareForShaderRead(Image *image, uint32_t baseLayer = 0, uint32_t layerCount = 0, uint32_t mipLevel = 0, uint32_t levelCount = 0);
        void PrepareForTransferSrc(Image *image, uint32_t baseLayer = 0, uint32_t layerCount = 0, uint32_t mipLevel = 0, uint32_t levelCount = 0);
        void PrepareForTransferDst(Image *image, uint32_t baseLayer = 0, uint32_t layerCount = 0, uint32_t mipLevel = 0, uint32_t levelCount = 0);
        void PrepareForPresentSrc(Image *image, uint32_t baseLayer = 0, uint32_t layerCount = 0, uint32_t mipLevel = 0, uint32_t levelCount = 0);
        void PrepareForBuffer(Buffer* buffer, VkPipelineStageFlags srcStages, VkPipelineStageFlags dstStages);

        Device* GetDevice() const { return device; }

        VkSubmitInfo GetSubmitInfo() const;

    private:
        void Reset();

    private:
        SmartPtr<Device> device;
        VkQueue queue = VK_NULL_HANDLE;

        // synchronization
        VkFence signalFence = VK_NULL_HANDLE;
        std::vector<VkSemaphore> waitSemaphores;
        std::vector<VkSemaphore> signalSemaphores;
        std::vector<VkPipelineStageFlags> waitStages;

        #ifndef NDEBUG
        // for validation purpose
        bool started = false;
        #endif

        std::vector<SmartPtr<StagingBuffer>> stagingBuffers;
    };

    template <typename T>
    void CommandBuffer::CopyDataToBuffer(const T &data, Buffer *buffer, size_t offset) {
        CopyDataToBuffer(const_cast<T*>(&data), sizeof(data), buffer, offset);
    }

    template <typename T>
    void CommandBuffer::CopyDataToBuffer(const std::vector<T> &data, Buffer *buffer, size_t offset) {
        CopyDataToBuffer(const_cast<T*>(data.data()), data.size() * sizeof(T), buffer, offset);
    }

    template <typename T>
    void CommandBuffer::CopyDataToImage(const std::vector<T> &data, Image *image,
                                        const VkOffset3D &offset, const VkExtent3D &extent,
                                        uint32_t baseLayer, uint32_t layerCount, uint32_t mipLevel,
                                        VkImageAspectFlags aspectMask) {
        CopyDataToImage(const_cast<T*>(data.data()), sizeof(T) * data.size(), image, offset, extent, baseLayer, layerCount, mipLevel, aspectMask);
    }

    // --------------------------------------------------------------------------------------------------

    class CommandPool final : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkCommandPool> {
    public:
        CommandPool(Device *device, uint32_t queueFamilyIndex);
        virtual ~CommandPool();

        void Reset();
        CommandBuffer* Request(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    private:
        CommandBuffer* RequestPrimaryCommandBuffer();
        CommandBuffer* RequestSecondaryCommandBuffer();

    private:
        SmartPtr<Device> device;
        VkQueue queue = VK_NULL_HANDLE;

        uint32_t activePrimaryCommandBuffers = 0;
        std::vector<SmartPtr<CommandBuffer>> primaryCommandBuffers;

        uint32_t activeSecondaryCommandBuffers = 0;
        std::vector<SmartPtr<CommandBuffer>> secondaryCommandBuffers;
    };


} // end of namespace slim

#endif // end of SLIM_CORE_COMMANDS_H
