#ifndef SLIM_CORE_RENDER_FRAME_H
#define SLIM_CORE_RENDER_FRAME_H

#include <string>
#include <vector>
#include <unordered_map>
#include <vulkan/vulkan.h>

#include "core/image.h"
#include "core/buffer.h"
#include "core/context.h"
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
    public:
        explicit RenderFrame(Context *context, uint32_t maxSetsPerPool = MAX_SETS_PER_POOL);
        explicit RenderFrame(Context *context, GPUImage2D *backBuffer, uint32_t maxSetsPerPool = MAX_SETS_PER_POOL);
        virtual ~RenderFrame();

        Context*                 GetContext() const { return context.get(); }
        GPUImage2D*              GetBackBuffer() const { return backBuffer.get(); };
        VkExtent2D               GetExtent() const { VkExtent3D extent = backBuffer->GetExtent(); return { extent.width, extent.height }; };

        Pipeline*                RequestPipeline(const std::string &name, const ComputePipelineDesc &desc);
        Pipeline*                RequestPipeline(const std::string &name, const GraphicsPipelineDesc &desc);
        Pipeline*                RequestPipeline(const std::string &name, const RayTracingPipelineDesc &desc);
        RenderPass*              RequestRenderPass(const std::string &name, const RenderPassDesc &renderPassDesc);
        Framebuffer*             RequestFramebuffer(const FramebufferDesc &framebufferDesc);
        CommandBuffer*           RequestCommandBuffer(VkQueueFlagBits queue, VkCommandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        VkDescriptorSet          RequestDescriptorSet(VkDescriptorSetLayout layout);
        Transient<GPUImage2D>    RequestGPUImage2D(VkFormat format, VkExtent2D extent, uint32_t mipLevels, VkSampleCountFlagBits samples, VkImageUsageFlags imageUsage);

        template <typename T>
        UniformBuffer*           RequestUniformBuffer(const T &value);

        void                     Reset();
        void                     Invalidate();
        void                     Present(CommandBuffer *commandBuffer);
        void                     SetBackBuffer(GPUImage2D *backBuffer);

    private:
        SmartPtr<Context>        context;
        SmartPtr<GPUImage2D>     backBuffer;
        QueueFamilyIndices       queueFamilyIndices;

        // queues
        SmartPtr<CommandPool>    computeCommandPools;
        SmartPtr<CommandPool>    graphicsCommandPools;
        SmartPtr<CommandPool>    transferCommandPools;

        // pools
        std::unique_ptr<Image2DPool<CPUImage2D>>   cpuImagePool;
        std::unique_ptr<Image2DPool<GPUImage2D>>   gpuImagePool;
        std::unique_ptr<BufferPool<UniformBuffer>> uniformBufferPool;
        std::unique_ptr<DescriptorPool>            descriptorPool;

        // mappings
        std::unordered_map<std::string, SmartPtr<Pipeline>> pipelines;
        std::unordered_map<std::string, SmartPtr<RenderPass>> renderPasses;
        std::unordered_map<std::size_t, SmartPtr<Framebuffer>> framebuffers;

        // synchronization between graphics queue and present queue
        SmartPtr<Semaphore>   imageAvailableSemaphore;
        SmartPtr<Semaphore>   renderFinishesSemaphore;

        // synchronization between CPU and GPU
        Fence*                inflightFence      = nullptr;

        uint32_t              swapchainIndex     = 0;
        VkSwapchainKHR        swapchain          = VK_NULL_HANDLE;
    };

    template <typename T>
    UniformBuffer* RenderFrame::RequestUniformBuffer(const T &value) {
        UniformBuffer* uniform = uniformBufferPool->Request(sizeof(T));
        uniform->SetData(value);
        return uniform;
    }

} // end of namespace slim

#endif // end of SLIM_CORE_RENDER_FRAME_H
