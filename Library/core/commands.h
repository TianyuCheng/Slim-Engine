#ifndef SLIM_CORE_COMMANDS_H
#define SLIM_CORE_COMMANDS_H

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "core/buffer.h"
#include "core/context.h"
#include "core/pipeline.h"
#include "core/synchronization.h"
#include "utility/interface.h"

namespace slim {

    class CommandPool;
    class CommandBuffer;

    class CommandBuffer final : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkCommandBuffer> {
        friend class CommandPool;
    public:
        using PoolType = CommandPool;
        explicit CommandBuffer(Context *context, VkQueue queue, VkCommandBuffer commandBuffer);
        virtual ~CommandBuffer();

        void Begin();
        void End();
        void Submit();
        void Wait(Semaphore *semaphore, VkPipelineStageFlags stages);
        void Signal(Semaphore *semaphore);
        void Signal(Fence *fence);

        void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance);

        void BindPipeline(Pipeline *pipeline);
        void BindDescriptor(Descriptor *descriptor);
        void BindIndexBuffer(IndexBuffer *buffer, size_t offset = 0);
        void BindVertexBuffer(uint32_t binding, VertexBuffer *buffer, uint64_t offset);
        void BindVertexBuffers(uint32_t binding, const std::vector<VertexBuffer*> &buffers, const std::vector<uint64_t> &offsets);

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

        void GenerateMipmaps(Image *image, VkFilter filter);
        void PrepareForShaderRead(Image *image);
        void PrepareForTransferSrc(Image *image);
        void PrepareForTransferDst(Image *image);
        void PrepareForPresentSrc(Image *image);
        void PrepareForMemoryMapping(Image *image);

        Context* GetContext() const { return context.get(); }

    private:
        void Reset();

    private:
        SmartPtr<Context> context;
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
        CopyDataToImage(const_cast<T*>(data.data()), image, offset, extent, baseLayer, layerCount, mipLevel, aspectMask);
    }

    // --------------------------------------------------------------------------------------------------

    class CommandPool final : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkCommandPool> {
    public:
        CommandPool(Context *context, uint32_t queueFamilyIndex);
        virtual ~CommandPool();

        void Reset();
        CommandBuffer* Request(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    private:
        CommandBuffer* RequestPrimaryCommandBufer();
        CommandBuffer* RequestSecondaryCommandBufer();

    private:
        SmartPtr<Context> context;
        VkQueue queue = VK_NULL_HANDLE;

        uint32_t activePrimaryCommandBuffers = 0;
        std::vector<SmartPtr<CommandBuffer>> primaryCommandBuffers;

        uint32_t activeSecondaryCommandBuffers = 0;
        std::vector<SmartPtr<CommandBuffer>> secondaryCommandBuffers;
    };


} // end of namespace slim

#endif // end of SLIM_CORE_COMMANDS_H
