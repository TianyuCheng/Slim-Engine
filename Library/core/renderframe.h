#ifndef SLIM_CORE_RENDER_FRAME_H
#define SLIM_CORE_RENDER_FRAME_H

#include <string>
#include <vector>
#include <unordered_map>

#include "core/vulkan.h"
#include "core/image.h"
#include "core/buffer.h"
#include "core/device.h"
#include "core/commands.h"
#include "core/pipeline.h"
#include "core/framebuffer.h"
#include "core/renderpass.h"
#include "core/synchronization.h"
#include "utility/transient.h"
#include "utility/interface.h"

namespace slim {

    class Window;

    constexpr static uint32_t MAX_SETS_PER_POOL = 256;

    // RenderFrame is responsible for storing frame-scoped data for rendering purpose.
    class RenderFrame final : public NotCopyable, public NotMovable, public ReferenceCountable {
        friend class Window;
        friend class Descriptor;
    public:
        explicit RenderFrame(Device* device, uint32_t maxSetsPerPool = MAX_SETS_PER_POOL);
        explicit RenderFrame(Device* device, GPUImage* backBuffer, uint32_t maxSetsPerPool = MAX_SETS_PER_POOL);
        virtual ~RenderFrame();

        Device*                  GetDevice() const { return device; }
        GPUImage*                GetBackBuffer() const { return backBuffer; };
        VkExtent2D               GetExtent() const { VkExtent3D extent = backBuffer->GetExtent(); return { extent.width, extent.height }; };
        float                    GetAspectRatio() const;
        DescriptorPool*          GetDescriptorPool() const;

        Pipeline*                RequestPipeline(const ComputePipelineDesc &desc);
        Pipeline*                RequestPipeline(const GraphicsPipelineDesc &desc, uint32_t subpass = 0);
        Pipeline*                RequestPipeline(const RayTracingPipelineDesc &desc);
        RenderPass*              RequestRenderPass(const RenderPassDesc &renderPassDesc);
        Framebuffer*             RequestFramebuffer(const FramebufferDesc &framebufferDesc);
        CommandBuffer*           RequestCommandBuffer(VkQueueFlagBits queue, VkCommandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        Transient<GPUImage>      RequestGPUImage(VkFormat format, VkExtent2D extent, uint32_t mipLevels, uint32_t arrayLayers, VkSampleCountFlagBits samples, VkImageUsageFlags imageUsage);
        Semaphore*               RequestSemaphore();

        UniformBuffer*           RequestUniformBuffer(size_t size);

        template <typename T>
        UniformBuffer*           RequestUniformBuffer(const T &value);

        template <typename T>
        UniformBuffer*           RequestUniformBuffer(const std::vector<T> &value);

        void                     Reset();
        void                     Invalidate();
        void                     Present(CommandBuffer *commandBuffer);
        void                     Draw(CommandBuffer *commandBuffer);
        void                     SetBackBuffer(GPUImage *backBuffer);
        Fence*                   GetGraphicsFinishFence();
        Fence*                   GetComputeFinishFence();

        bool                     Presentable() const { return swapchain != VK_NULL_HANDLE; }

    private:
        SmartPtr<Device>         device;
        Window*                  window;
        SmartPtr<GPUImage>       backBuffer;
        QueueFamilyIndices       queueFamilyIndices;

        // queues
        SmartPtr<CommandPool>    computeCommandPools;
        SmartPtr<CommandPool>    graphicsCommandPools;
        SmartPtr<CommandPool>    transferCommandPools;

        // pools
        SmartPtr<ImagePool<CPUImage>>       cpuImagePool;
        SmartPtr<ImagePool<GPUImage>>       gpuImagePool;
        SmartPtr<BufferPool<UniformBuffer>> uniformBufferPool;
        SmartPtr<DescriptorPool>            descriptorPool;
        std::vector<SmartPtr<Semaphore>>    semaphorePool;
        uint32_t                            activeSemahoreCount = 0;

        // mappings
        std::unordered_map<std::string, SmartPtr<Pipeline>> pipelines;
        std::unordered_map<std::string, SmartPtr<RenderPass>> renderPasses;
        std::unordered_map<std::size_t, SmartPtr<Framebuffer>> framebuffers;

        // synchronization between graphics queue and present queue
        SmartPtr<Semaphore>   imageAvailableSemaphore;
        SmartPtr<Semaphore>   renderFinishesSemaphore;

        // synchronization between CPU and GPU
        Fence*                inflightFence       = nullptr;
        SmartPtr<Fence>       computeFinishFence  = nullptr;
        SmartPtr<Fence>       graphicsFinishFence = nullptr;

        uint32_t              swapchainIndex     = 0;
        VkSwapchainKHR        swapchain          = VK_NULL_HANDLE;
    };

    template <typename T>
    UniformBuffer* RenderFrame::RequestUniformBuffer(const T &value) {
        UniformBuffer* uniform = uniformBufferPool->Request(sizeof(T));
        uniform->SetData(value);
        return uniform;
    }

    template <typename T>
    UniformBuffer* RenderFrame::RequestUniformBuffer(const std::vector<T> &value) {
        UniformBuffer* uniform = uniformBufferPool->Request(sizeof(T) * value.size());
        uniform->SetData(value);
        return uniform;
    }

} // end of namespace slim

#endif // end of SLIM_CORE_RENDER_FRAME_H
