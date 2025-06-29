#include "core/debug.h"
#include "core/window.h"
#include "core/renderframe.h"
#include "core/vkutils.h"

using namespace slim;

RenderFrame::RenderFrame(Device* device, uint32_t maxSetsPerPool) : device(device) {
    queueFamilyIndices = device->GetQueueFamilyIndices();

    // initialize command pools
    if (queueFamilyIndices.compute.has_value())
        computeCommandPools = SlimPtr<CommandPool>(device, queueFamilyIndices.compute.value());

    if (queueFamilyIndices.graphics.has_value())
        graphicsCommandPools = SlimPtr<CommandPool>(device, queueFamilyIndices.graphics.value());

    if (queueFamilyIndices.transfer.has_value())
        transferCommandPools = SlimPtr<CommandPool>(device, queueFamilyIndices.transfer.value());

    // initialize pools for resource allocation
    cpuImagePool = SlimPtr<ImagePool<CPUImage>>(device);
    gpuImagePool = SlimPtr<ImagePool<GPUImage>>(device);
    uniformBufferPool = SlimPtr<BufferPool<UniformBuffer>>(device);
    descriptorPool = SlimPtr<DescriptorPool>(device, maxSetsPerPool);

    // initialize synchronization objects
    imageAvailableSemaphore = SlimPtr<Semaphore>(device);
    renderFinishesSemaphore = SlimPtr<Semaphore>(device);
}

RenderFrame::RenderFrame(Device* device, GPUImage* backBuffer, uint32_t maxSetsPerPool) : RenderFrame(device, maxSetsPerPool) {
    this->backBuffer.reset(backBuffer);
}

RenderFrame::~RenderFrame() {
    device->WaitIdle();
    Invalidate();
    Reset();
}

void RenderFrame::Reset() {
    if (computeFinishFence) {
        computeFinishFence->Wait();
        computeFinishFence->Reset();
    }

    if (graphicsFinishFence) {
        graphicsFinishFence->Wait();
        graphicsFinishFence->Reset();
    }

    // reset all command pools
    if (computeCommandPools.get())  computeCommandPools->Reset();
    if (graphicsCommandPools.get()) graphicsCommandPools->Reset();
    if (transferCommandPools.get()) transferCommandPools->Reset();

    uniformBufferPool->Reset();
    descriptorPool->Reset();
    activeSemahoreCount = 0;
    semaphorePool.clear();
}

void RenderFrame::Invalidate() {
    framebuffers.clear();
    renderPasses.clear();
    pipelines.clear();
}

void RenderFrame::SetBackBuffer(GPUImage *backBuffer) {
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

    commandBuffer->Wait(imageAvailableSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    commandBuffer->Signal(renderFinishesSemaphore);
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
    VkResult result = vkQueuePresentKHR(device->GetPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // recreation swapchain
        window->OnResize();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image!");
    }
}

void RenderFrame::Draw(CommandBuffer *commandBuffer) {
    commandBuffer->Signal(renderFinishesSemaphore);
    commandBuffer->Submit();
}

Pipeline* RenderFrame::RequestPipeline(const ComputePipelineDesc &desc) {
    const std::string &name = desc.GetName();
    auto it = pipelines.find(name);
    if (it == pipelines.end()) {
        pipelines.insert(std::make_pair(name, SlimPtr<Pipeline>(device, desc)));
        it = pipelines.find(name);
    }
    return it->second;
}

Pipeline* RenderFrame::RequestPipeline(const GraphicsPipelineDesc &desc, uint32_t subpass) {
    const std::string &name = desc.GetName();
    auto it = pipelines.find(name);
    if (it == pipelines.end()) {
        pipelines.insert(std::make_pair(name, SlimPtr<Pipeline>(device, desc, subpass)));
        it = pipelines.find(name);
    }
    return it->second;
}

Pipeline* RenderFrame::RequestPipeline(const RayTracingPipelineDesc &desc) {
    const std::string &name = desc.GetName();
    auto it = pipelines.find(name);
    if (it == pipelines.end()) {
        pipelines.insert(std::make_pair(name, SlimPtr<Pipeline>(device, desc)));
        it = pipelines.find(name);
    }
    return it->second;
}

RenderPass* RenderFrame::RequestRenderPass(const RenderPassDesc &desc) {
    const std::string &name = desc.GetName();
    auto it = renderPasses.find(name);
    if (it == renderPasses.end()) {
        renderPasses.insert(std::make_pair(name, SlimPtr<RenderPass>(device, desc)));
        it = renderPasses.find(name);
    }
    return it->second;
}

Framebuffer* RenderFrame::RequestFramebuffer(const FramebufferDesc &desc) {
    size_t hash = std::hash<FramebufferDesc>{}(desc);
    auto it = framebuffers.find(hash);
    if (it == framebuffers.end()) {
        framebuffers.insert(std::make_pair(hash, SlimPtr<Framebuffer>(device, desc)));
        it = framebuffers.find(hash);
    }
    return it->second;
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

Transient<GPUImage> RenderFrame::RequestGPUImage(VkFormat format, VkExtent2D extent, uint32_t mipLevels, uint32_t arrayLayers, VkSampleCountFlagBits samples, VkImageUsageFlags imageUsage) {
    return gpuImagePool->Request(format, extent, mipLevels, arrayLayers, samples, imageUsage);
}

UniformBuffer* RenderFrame::RequestUniformBuffer(size_t size) {
    return uniformBufferPool->Request(size);
}

Semaphore* RenderFrame::RequestSemaphore() {
    if (activeSemahoreCount < semaphorePool.size()) {
        return semaphorePool[activeSemahoreCount++];
    }

    semaphorePool.push_back(SlimPtr<Semaphore>(device));
    return semaphorePool[activeSemahoreCount++];
}

Fence* RenderFrame::GetComputeFinishFence() {
    if (!computeFinishFence) {
        computeFinishFence = SlimPtr<Fence>(device);
    }
    return computeFinishFence;
}

Fence* RenderFrame::GetGraphicsFinishFence() {
    if (!graphicsFinishFence) {
        graphicsFinishFence = SlimPtr<Fence>(device);
    }
    return graphicsFinishFence;
}
