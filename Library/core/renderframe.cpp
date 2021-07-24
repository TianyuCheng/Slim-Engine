#include "core/debug.h"
#include "core/renderframe.h"
#include "core/vkutils.h"

using namespace slim;

RenderFrame::RenderFrame(Context *context, uint32_t maxSetsPerPool) : context(context) {
    queueFamilyIndices = context->GetQueueFamilyIndices();

    // initialize command pools
    if (queueFamilyIndices.compute.has_value())
        computeCommandPools = SlimPtr<CommandPool>(context, queueFamilyIndices.compute.value());

    if (queueFamilyIndices.graphics.has_value())
        graphicsCommandPools = SlimPtr<CommandPool>(context, queueFamilyIndices.graphics.value());

    if (queueFamilyIndices.transfer.has_value())
        transferCommandPools = SlimPtr<CommandPool>(context, queueFamilyIndices.transfer.value());

    // initialize pools for resource allocation
    cpuImagePool = SlimPtr<Image2DPool<CPUImage2D>>(context);
    gpuImagePool = SlimPtr<Image2DPool<GPUImage2D>>(context);
    uniformBufferPool = SlimPtr<BufferPool<UniformBuffer>>(context);
    descriptorPool = SlimPtr<DescriptorPool>(context, maxSetsPerPool);

    // initialize synchronization objects
    imageAvailableSemaphore = SlimPtr<Semaphore>(context);
    renderFinishesSemaphore = SlimPtr<Semaphore>(context);
}

RenderFrame::RenderFrame(Context *context, GPUImage2D *backBuffer, uint32_t maxSetsPerPool) : RenderFrame(context, maxSetsPerPool) {
    this->backBuffer.reset(backBuffer);
}

RenderFrame::~RenderFrame() {
    context->WaitIdle();
    Invalidate();
    Reset();
}

void RenderFrame::Reset() {
    // reset all command pools
    if (computeCommandPools.get())  computeCommandPools->Reset();
    if (graphicsCommandPools.get()) graphicsCommandPools->Reset();
    if (transferCommandPools.get()) transferCommandPools->Reset();
    uniformBufferPool->Reset();
}

void RenderFrame::Invalidate() {
    framebuffers.clear();
    renderPasses.clear();
    pipelines.clear();
}

void RenderFrame::SetBackBuffer(GPUImage2D *backBuffer) {
    this->backBuffer.reset(backBuffer);
}

float RenderFrame::GetAspectRatio() const {
    const VkExtent3D &extent = backBuffer->GetExtent();
    return static_cast<float>(extent.width) / static_cast<float>(extent.height);
}

DescriptorPool* RenderFrame::GetDescriptorPool() const {
    return descriptorPool;
}

void RenderFrame::Present(CommandBuffer *commandBuffer) {
    VkSemaphore signalSemaphoreVk = *renderFinishesSemaphore;

    commandBuffer->Wait(imageAvailableSemaphore.get(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    commandBuffer->Signal(renderFinishesSemaphore.get());
    commandBuffer->Signal(inflightFence);
    commandBuffer->Submit();

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &swapchainIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &signalSemaphoreVk;
    presentInfo.pResults = VK_NULL_HANDLE;
    ErrorCheck(vkQueuePresentKHR(context->presentQueue, &presentInfo), "present a frame");
}

Pipeline* RenderFrame::RequestPipeline(const ComputePipelineDesc &desc) {
    const std::string &name = desc.GetName();
    auto it = pipelines.find(name);
    if (it == pipelines.end()) {
        pipelines.insert(std::make_pair(name, SlimPtr<Pipeline>(context.get(), desc)));
        it = pipelines.find(name);
    }
    return it->second.get();
}

Pipeline* RenderFrame::RequestPipeline(const GraphicsPipelineDesc &desc) {
    const std::string &name = desc.GetName();
    auto it = pipelines.find(name);
    if (it == pipelines.end()) {
        pipelines.insert(std::make_pair(name, SlimPtr<Pipeline>(context.get(), desc)));
        it = pipelines.find(name);
    }
    return it->second.get();
}

Pipeline* RenderFrame::RequestPipeline(const RayTracingPipelineDesc &desc) {
    const std::string &name = desc.GetName();
    auto it = pipelines.find(name);
    if (it == pipelines.end()) {
        pipelines.insert(std::make_pair(name, SlimPtr<Pipeline>(context.get(), desc)));
        it = pipelines.find(name);
    }
    return it->second.get();
}

RenderPass* RenderFrame::RequestRenderPass(const RenderPassDesc &desc) {
    const std::string &name = desc.GetName();
    auto it = renderPasses.find(name);
    if (it == renderPasses.end()) {
        renderPasses.insert(std::make_pair(name, SlimPtr<RenderPass>(context.get(), desc)));
        it = renderPasses.find(name);
    }
    return it->second.get();
}

Framebuffer* RenderFrame::RequestFramebuffer(const FramebufferDesc &desc) {
    size_t hash = std::hash<FramebufferDesc>{}(desc);
    auto it = framebuffers.find(hash);
    if (it == framebuffers.end()) {
        framebuffers.insert(std::make_pair(hash, SlimPtr<Framebuffer>(context.get(), desc)));
        it = framebuffers.find(hash);
    }
    return it->second.get();
}

CommandBuffer* RenderFrame::RequestCommandBuffer(VkQueueFlagBits queue, VkCommandBufferLevel level) {
    switch (queue) {
        case VK_QUEUE_COMPUTE_BIT:
            return computeCommandPools->Request(level);
        case VK_QUEUE_GRAPHICS_BIT:
            return graphicsCommandPools->Request(level);
        case VK_QUEUE_TRANSFER_BIT:
            return transferCommandPools->Request(level);
        default:
            throw std::runtime_error("Fail to request command buffer other than compute/graphics/transfer");
    }
}

Transient<GPUImage2D> RenderFrame::RequestGPUImage2D(VkFormat format, VkExtent2D extent, uint32_t mipLevels, VkSampleCountFlagBits samples, VkImageUsageFlags imageUsage) {
    return gpuImagePool->Request(format, extent, mipLevels, samples, imageUsage);
}
