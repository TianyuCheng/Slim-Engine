#include "core/debug.h"
#include "core/commands.h"
#include "core/vkutils.h"
#include "utility/stb.h"

using namespace slim;

CommandBuffer::CommandBuffer(Device *device, VkQueue queue, VkCommandBuffer commandBuffer)
    : device(device), queue(queue) {
    handle = commandBuffer;
}

CommandBuffer::~CommandBuffer() {
    Reset();
    queue = VK_NULL_HANDLE;
    handle = VK_NULL_HANDLE;
}

void CommandBuffer::Reset() {
    stagingBuffers.clear();
    waitSemaphores.clear();
    signalSemaphores.clear();
    waitStages.clear();
}

void CommandBuffer::Begin() {
    #ifndef NDEBUG
    // for validation purpose
    if (started) {
        throw std::runtime_error("CommandBuffer has already begun when calling Begin()! Need to call End()");
    }
    started = true;
    #endif

    // only use one time submit
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    ErrorCheck(vkBeginCommandBuffer(handle, &beginInfo), "begin command buffer");
}

void CommandBuffer::End() {
    #ifndef NDEBUG
    // for validation purpose
    if (!started) {
        throw std::runtime_error("CommandBuffer has not begun when calling End()! Need to call Begin()");
    }
    started = false;
    #endif
    ErrorCheck(vkEndCommandBuffer(handle), "end command buffer")
}

void CommandBuffer::NextSubpass(VkSubpassContents contents) {
    DeviceDispatch(vkCmdNextSubpass(handle, contents));
}

void CommandBuffer::Submit() {
    // prepare submit info
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;

    // wait semaphores
    submitInfo.waitSemaphoreCount = waitSemaphores.size();
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();

    // signal semaphores
    submitInfo.signalSemaphoreCount = signalSemaphores.size();
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    // command buffer
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &handle;

    // ErrorCheck(DeviceDispatch(vkQueueSubmit(queue, 1, &submitInfo, signalFence)), "submit command buffer");
    DeviceDispatch(vkQueueSubmit(queue, 1, &submitInfo, signalFence));

    // clean up
    waitSemaphores.clear();
    waitStages.clear();
    signalSemaphores.clear();
    signalFence = VK_NULL_HANDLE;
}

void CommandBuffer::Wait(Semaphore *semaphore, VkPipelineStageFlags stages) {
    VkSemaphore sema = *semaphore;
    auto it = std::find(waitSemaphores.begin(), waitSemaphores.end(), sema);
    if (it == waitSemaphores.end()) {
        waitSemaphores.push_back(sema);
        waitStages.push_back(stages);
    }
}

void CommandBuffer::Signal(Semaphore *semaphore) {
    VkSemaphore sema = *semaphore;
    auto it = std::find(signalSemaphores.begin(), signalSemaphores.end(), sema);
    if (it == signalSemaphores.end()) {
        signalSemaphores.push_back(sema);
    }
}

void CommandBuffer::Signal(Fence *fence) {
    signalFence = *fence;
}

VkSubmitInfo CommandBuffer::GetSubmitInfo() const {
    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &handle;
    submit.signalSemaphoreCount = signalSemaphores.size();
    submit.pSignalSemaphores = signalSemaphores.data();
    submit.waitSemaphoreCount = waitSemaphores.size();
    submit.pWaitSemaphores = waitSemaphores.data();
    submit.pWaitDstStageMask = waitStages.data();
    submit.pNext = nullptr;
    return submit;
}

void CommandBuffer::CopyDataToBuffer(void *data, size_t size, Buffer *buffer, size_t offset) {
    // host visible
    // memory mapped
    if (buffer->HostVisible()) {
        buffer->SetData(data, size, offset);
    }

    // buffer update
    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkCmdUpdateBuffer.html
    else if (size <= 65536 && size % 4 == 0 && offset % 4 == 0) {
        DeviceDispatch(vkCmdUpdateBuffer(handle, *buffer, offset, size, data));
    }

    // use staging buffer
    // fallback
    else {
        stagingBuffers.emplace_back(new StagingBuffer(device, size));
        auto staging = stagingBuffers.back().get();
        staging->SetData(data, size);
        CopyBufferToBuffer(staging, 0, buffer, offset, size);
    }
}

void CommandBuffer::CopyDataToImage(void *data, size_t size, Image *image,
                                    const VkOffset3D &offset, const VkExtent3D &extent,
                                    uint32_t baseLayer, uint32_t layerCount, uint32_t mipLevel,
                                    VkImageAspectFlags aspectMask) {
    PrepareForTransferDst(image, baseLayer, layerCount, mipLevel, 1);

    stagingBuffers.emplace_back(new StagingBuffer(device, size));
    auto stagingBuffer = stagingBuffers.back().get();
    stagingBuffer->SetData(data, size);
    CopyBufferToImage(stagingBuffer, 0, 0, extent.height, image, offset, extent, baseLayer, layerCount, mipLevel, aspectMask);
}

void CommandBuffer::CopyBufferToBuffer(Buffer *srcBuffer, size_t srcOffset, Buffer *dstBuffer, size_t dstOffset, size_t size) {
    VkBufferCopy copy = {};
    copy.srcOffset = srcOffset;
    copy.dstOffset = dstOffset;
    copy.size = size;
    DeviceDispatch(vkCmdCopyBuffer(handle, *srcBuffer, *dstBuffer, 1, &copy));
}

void CommandBuffer::CopyBufferToImage(Buffer *srcBuffer, size_t bufferOffset, size_t bufferRowLength, size_t bufferImageHeight,
                                      Image *dstImage, const VkOffset3D &offset, const VkExtent3D &extent,
                                      uint32_t baseLayer, uint32_t layerCount, uint32_t mipLevel, VkImageAspectFlags aspectMask) {
    PrepareForTransferDst(dstImage, baseLayer, layerCount, mipLevel, 1);

    VkBufferImageCopy copy = {};
    copy.bufferOffset = bufferOffset;
    copy.bufferRowLength = bufferRowLength;
    copy.bufferImageHeight = bufferImageHeight;
    copy.imageOffset = offset;
    copy.imageExtent = extent;
    copy.imageSubresource.aspectMask = aspectMask;
    copy.imageSubresource.baseArrayLayer = baseLayer;
    copy.imageSubresource.layerCount = layerCount;
    copy.imageSubresource.mipLevel = mipLevel;

    DeviceDispatch(vkCmdCopyBufferToImage(handle, *srcBuffer, *dstImage, dstImage->layouts[baseLayer][mipLevel], 1, &copy));
}

void CommandBuffer::CopyImageToBuffer(Image *srcImage, const VkOffset3D &offset, const VkExtent3D &extent,
                                      uint32_t baseLayer, uint32_t layerCount, uint32_t mipLevel, VkImageAspectFlags aspectMask,
                                      Buffer *dstBuffer, size_t bufferOffset, size_t bufferRowLength, size_t bufferImageHeight) {
    PrepareForTransferSrc(srcImage, baseLayer, layerCount, mipLevel, 1);

    VkBufferImageCopy copy = {};
    copy.bufferOffset = bufferOffset;
    copy.bufferRowLength = bufferRowLength;
    copy.bufferImageHeight = bufferImageHeight;
    copy.imageOffset = offset;
    copy.imageExtent = extent;
    copy.imageSubresource.aspectMask = aspectMask;
    copy.imageSubresource.baseArrayLayer = baseLayer;
    copy.imageSubresource.layerCount = layerCount;
    copy.imageSubresource.mipLevel = mipLevel;

    DeviceDispatch(vkCmdCopyImageToBuffer(handle, *srcImage, srcImage->layouts[baseLayer][mipLevel], *dstBuffer, 1, &copy));
}

void CommandBuffer::CopyImageToImage(Image *srcImage, const VkOffset3D &srcOffset,
                                     uint32_t srcBaseLayer, uint32_t srcLayerCount,
                                     uint32_t srcMipLevel, VkImageAspectFlags srcAspectMask,
                                     Image *dstImage, const VkOffset3D &dstOffset,
                                     uint32_t dstBaseLayer, uint32_t dstLayerCount,
                                     uint32_t dstMipLevel, VkImageAspectFlags dstAspectMask) {
    PrepareForTransferSrc(srcImage, srcBaseLayer, srcLayerCount, srcMipLevel, 1);
    PrepareForTransferDst(dstImage, dstBaseLayer, dstLayerCount, dstMipLevel, 1);

    VkImageCopy copy = {};
    copy.srcOffset = srcOffset;
    copy.srcSubresource.aspectMask = srcAspectMask;
    copy.srcSubresource.baseArrayLayer = srcBaseLayer;
    copy.srcSubresource.layerCount = srcLayerCount;
    copy.srcSubresource.mipLevel = srcMipLevel;
    copy.dstOffset = dstOffset;
    copy.dstSubresource.aspectMask = dstAspectMask;
    copy.dstSubresource.baseArrayLayer = dstBaseLayer;
    copy.dstSubresource.layerCount = dstLayerCount;
    copy.dstSubresource.mipLevel = dstMipLevel;
    copy.extent = srcImage->GetExtent();
    DeviceDispatch(vkCmdCopyImage(handle,
                   *srcImage, srcImage->layouts[srcBaseLayer][srcMipLevel],
                   *dstImage, dstImage->layouts[srcBaseLayer][srcMipLevel],
                   1, &copy));
}

void CommandBuffer::BlitImage(Image *srcImage, const VkOffset3D &srcOffset1, const VkOffset3D &srcOffset2,
                              uint32_t srcBaseLayer, uint32_t srcLayerCount, uint32_t srcMipLevel, VkImageAspectFlags srcAspectMask,
                              Image *dstImage, const VkOffset3D &dstOffset1, const VkOffset3D &dstOffset2,
                              uint32_t dstBaseLayer, uint32_t dstLayerCount, uint32_t dstMipLevel, VkImageAspectFlags dstAspectMask,
                              VkFilter filter) {
    VkImageBlit blit = {};
    blit.srcOffsets[0] = srcOffset1;
    blit.srcOffsets[1] = srcOffset2;
    blit.srcSubresource.aspectMask = srcAspectMask;
    blit.srcSubresource.baseArrayLayer = srcBaseLayer;
    blit.srcSubresource.layerCount = srcLayerCount;
    blit.srcSubresource.mipLevel = srcMipLevel;
    blit.dstOffsets[0] = dstOffset1;
    blit.dstOffsets[1] = dstOffset2;
    blit.dstSubresource.aspectMask = dstAspectMask;
    blit.dstSubresource.baseArrayLayer = dstBaseLayer;
    blit.dstSubresource.layerCount = dstLayerCount;
    blit.dstSubresource.mipLevel = dstMipLevel;
    DeviceDispatch(vkCmdBlitImage(handle,
                   *srcImage, srcImage->layouts[srcBaseLayer][srcMipLevel],
                   *dstImage, dstImage->layouts[dstBaseLayer][dstMipLevel],
                   1, &blit, filter));
}

void CommandBuffer::GenerateMipmaps(Image *image, VkFilter filter) {
    // no need to generate mipmaps
    if (image->MipLevels() <= 1) return;

    for (uint32_t layer = 0; layer < image->Layers(); layer++) {
        int32_t mipW = image->Width();
        int32_t mipH = image->Height();

        for (uint32_t i = 0; i < image->MipLevels() - 1; i++) {
            // transit mip-level {i} to transfer src
            PrepareLayoutTransition(handle, image,
                                    image->layouts[layer][i],
                                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                                    layer, 1, i, 1);

            // transit mip-level {i+1} to transfer dst
            PrepareLayoutTransition(handle, image,
                                    image->layouts[layer][i + 1],
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                                    layer, 1, i + 1, 1);

            // blit image
            int32_t mipWNext = (mipW >> 1) > 0 ? (mipW >> 1) : 1;
            int32_t mipHNext = (mipH >> 1) > 0 ? (mipH >> 1) : 1;

            BlitImage(image, VkOffset3D { 0, 0, 0 }, VkOffset3D { mipW,     mipH,     1 }, layer, 1, i, VK_IMAGE_ASPECT_COLOR_BIT,
                      image, VkOffset3D { 0, 0, 0 }, VkOffset3D { mipWNext, mipHNext, 1 }, layer, 1, i + 1, VK_IMAGE_ASPECT_COLOR_BIT,
                      filter);

            mipW = mipWNext;
            mipH = mipHNext;
        }

        // transit mip-level mipCount-1 to transfer src
        uint32_t mip = image->MipLevels() - 1;
        PrepareLayoutTransition(handle, image,
                                image->layouts[layer][mip],
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                layer, 1, mip, 1);
    }
}

void CommandBuffer::PrepareForShaderRead(Image *image, uint32_t baseLayer, uint32_t layerCount, uint32_t mipLevel, uint32_t levelCount) {
    if (layerCount == 0) layerCount = image->createInfo.arrayLayers;
    if (levelCount == 0) levelCount = image->createInfo.mipLevels;
    // transit dst image layout
    PrepareLayoutTransition(handle, image,
        image->layouts[baseLayer][mipLevel],
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        baseLayer, layerCount,
        mipLevel, levelCount);
}

void CommandBuffer::PrepareForTransferSrc(Image *image, uint32_t baseLayer, uint32_t layerCount, uint32_t mipLevel, uint32_t levelCount) {
    if (layerCount == 0) layerCount = image->createInfo.arrayLayers;
    if (levelCount == 0) levelCount = image->createInfo.mipLevels;
    // transit dst image layout
    PrepareLayoutTransition(handle, image,
        image->layouts[baseLayer][mipLevel],
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        baseLayer, layerCount,
        mipLevel, levelCount);
}

void CommandBuffer::PrepareForTransferDst(Image *image, uint32_t baseLayer, uint32_t layerCount, uint32_t mipLevel, uint32_t levelCount) {
    if (layerCount == 0) layerCount = image->createInfo.arrayLayers;
    if (levelCount == 0) levelCount = image->createInfo.mipLevels;
    // transit dst image layout
    PrepareLayoutTransition(handle, image,
        image->layouts[baseLayer][mipLevel],
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        baseLayer, layerCount,
        mipLevel, levelCount);
}

void CommandBuffer::PrepareForPresentSrc(Image *image, uint32_t baseLayer, uint32_t layerCount, uint32_t mipLevel, uint32_t levelCount) {
    if (layerCount == 0) layerCount = image->createInfo.arrayLayers;
    if (levelCount == 0) levelCount = image->createInfo.mipLevels;
    // transit dst image layout
    PrepareLayoutTransition(handle, image,
        image->layouts[baseLayer][mipLevel],
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        baseLayer, layerCount,
        mipLevel, levelCount);
}

void CommandBuffer::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
    DeviceDispatch(vkCmdDispatch(handle, x, y, z));
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    DeviceDispatch(vkCmdDraw(handle, vertexCount, instanceCount, firstVertex, firstInstance));
}

void CommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) {
    DeviceDispatch(vkCmdDrawIndexed(handle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance));
}

void CommandBuffer::BindPipeline(Pipeline *pipeline) {
    DeviceDispatch(vkCmdBindPipeline(handle, pipeline->Type(), *pipeline));
}

void CommandBuffer::BindDescriptor(Descriptor *descriptor, VkPipelineBindPoint bindPoint) {
    descriptor->Update();

    VkPipelineLayout layout = *descriptor->pipelineLayout;

    // bind descriptor set individually (need to optimize it later)
    // TODO: batch descriptor sets binding if possible
    for (uint32_t i = 0; i < descriptor->descriptorSets.size(); i++) {
        if (descriptor->descriptorSets[i] != VK_NULL_HANDLE) {
            uint32_t dynamicOffsetCount = descriptor->dynamicOffsets[i].size();
            uint32_t* dynamicOffsetData = descriptor->dynamicOffsets[i].data();
            DeviceDispatch(vkCmdBindDescriptorSets(handle, bindPoint, layout, i, 1, &descriptor->descriptorSets[i], dynamicOffsetCount, dynamicOffsetData));
        }
    }
}

void CommandBuffer::BindIndexBuffer(IndexBuffer *buffer, size_t offset) {
    DeviceDispatch(vkCmdBindIndexBuffer(handle, *buffer, offset, buffer->indexType));
}

void CommandBuffer::BindIndexBuffer(Buffer *buffer, size_t offset, VkIndexType indexType) {
    DeviceDispatch(vkCmdBindIndexBuffer(handle, *buffer, offset, indexType));
}

void CommandBuffer::BindVertexBuffer(uint32_t binding, Buffer *buffer, uint64_t offset) {
    VkBuffer vBuffer = *buffer;
    DeviceDispatch(vkCmdBindVertexBuffers(handle, binding, 1, &vBuffer, &offset));
}

void CommandBuffer::BindVertexBuffers(uint32_t binding, const std::vector<Buffer*> &buffers, const std::vector<uint64_t> &offsets) {
    std::vector<VkBuffer> vBuffers;
    for (auto buffer : buffers)
        vBuffers.push_back(*buffer);
    DeviceDispatch(vkCmdBindVertexBuffers(handle, binding, vBuffers.size(), vBuffers.data(), offsets.data()));
}

void CommandBuffer::PushConstants(PipelineLayout *layout, const std::string &name, const void *value) {
    const VkPushConstantRange& range = layout->GetPushConstant(name);
    PushConstants(layout, range.offset, value, range.size, range.stageFlags);
}

void CommandBuffer::PushConstants(PipelineLayout *layout, size_t offset, const void *value, size_t size, VkShaderStageFlags stages) {
    DeviceDispatch(vkCmdPushConstants(handle, *layout, stages, offset, size, value));
}

CommandPool::CommandPool(Device *device, uint32_t queueFamilyIndex) : device(device) {
    // get device queue
    vkGetDeviceQueue(*device, queueFamilyIndex, 0, &queue);

    // create command pool
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = 0;
    createInfo.queueFamilyIndex = queueFamilyIndex;
    createInfo.pNext = nullptr;

    // error checking
    ErrorCheck(DeviceDispatch(vkCreateCommandPool(*device, &createInfo, nullptr, &handle)), "create command pool");
}

CommandPool::~CommandPool() {
    Reset();

    primaryCommandBuffers.clear();
    secondaryCommandBuffers.clear();

    if (handle) {
        vkDestroyCommandPool(*device, handle, nullptr);
        handle = nullptr;
    }
}

void CommandPool::Reset() {
    // always use pool to reset all command buffers
    ErrorCheck(vkResetCommandPool(*device, handle, 0), "reset command pool");

    // clearing staging buffers
    for (auto commandBuffer : primaryCommandBuffers) commandBuffer->Reset();
    for (auto commandBuffer : secondaryCommandBuffers) commandBuffer->Reset();

    activePrimaryCommandBuffers = 0;
    activeSecondaryCommandBuffers = 0;
}

CommandBuffer* CommandPool::Request(VkCommandBufferLevel level) {
    CommandBuffer *commandBuffer = level == VK_COMMAND_BUFFER_LEVEL_PRIMARY
                                 ? RequestPrimaryCommandBuffer()
                                 : RequestSecondaryCommandBuffer();
    return commandBuffer;
}

CommandBuffer* CommandPool::RequestPrimaryCommandBuffer() {
    if (activePrimaryCommandBuffers < primaryCommandBuffers.size()) {
        return primaryCommandBuffers[activePrimaryCommandBuffers++].get();
    }

    // command buffer allocation info
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.commandPool = handle;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    // allocate command buffer
    VkCommandBuffer commandBuffer;
    ErrorCheck(DeviceDispatch(vkAllocateCommandBuffers(*device, &allocInfo, &commandBuffer)), "allocate command buffer");

    // adding a new command buffer
    primaryCommandBuffers.push_back(SlimPtr<CommandBuffer>(device, queue, commandBuffer));
    activePrimaryCommandBuffers++;
    return primaryCommandBuffers.back().get();
}

CommandBuffer* CommandPool::RequestSecondaryCommandBuffer() {
    if (activeSecondaryCommandBuffers < secondaryCommandBuffers.size()) {
        return secondaryCommandBuffers[activeSecondaryCommandBuffers++].get();
    }

    // command buffer allocation info
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.commandPool = handle;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = 1;

    // allocate command buffer
    VkCommandBuffer commandBuffer;
    ErrorCheck(DeviceDispatch(vkAllocateCommandBuffers(*device, &allocInfo, &commandBuffer)), "allocate command buffer")

    // adding a new command buffer
    secondaryCommandBuffers.push_back(SlimPtr<CommandBuffer>(device, queue, commandBuffer));
    activeSecondaryCommandBuffers++;
    return secondaryCommandBuffers.back().get();
}

void CommandBuffer::SetName(const std::string& name) const {
    if (device->IsDebuggerEnabled()) {
        VkDebugMarkerObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT;
        nameInfo.object = (uint64_t) handle;
        nameInfo.pObjectName = name.c_str();
        ErrorCheck(DeviceDispatch(vkDebugMarkerSetObjectNameEXT(*device, &nameInfo)), "set query pool name");
    }
}

void CommandBuffer::BeginRegion(const std::string& name) const {
    if (device->IsDebuggerEnabled()) {
        VkDebugMarkerMarkerInfoEXT markerInfo = {};
        markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
        // Color to display this region with (if supported by debugger)
        markerInfo.color[0] = 1.0f;
        markerInfo.color[1] = 1.0f;
        markerInfo.color[2] = 0.0f;
        markerInfo.color[3] = 1.0f;
        // Name of the region displayed by the debugging application
        markerInfo.pMarkerName = name.c_str();
        DeviceDispatch(vkCmdDebugMarkerBeginEXT(handle, &markerInfo));
    }
}

void CommandBuffer::EndRegion() const {
    if (device->IsDebuggerEnabled()) {
        DeviceDispatch(vkCmdDebugMarkerEndEXT(handle));
    }
}

void CommandBuffer::InsertMarker(const std::string& marker) const {
    VkDebugMarkerMarkerInfoEXT markerInfo = {};
    markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
    markerInfo.pMarkerName = marker.c_str();
    // Color to display this marker with (if supported by debugger)
    markerInfo.color[0] = 1.0f;
    markerInfo.color[1] = 1.0f;
    markerInfo.color[2] = 0.0f;
    markerInfo.color[3] = 1.0f;
    DeviceDispatch(vkCmdDebugMarkerInsertEXT(handle, &markerInfo));
}

void CommandBuffer::ClearColor(Image* image,
                               const VkClearColorValue& clear,
                               uint32_t baseLayer, uint32_t layerCount,
                               uint32_t mipLevel, uint32_t levelCount) {
    if (layerCount == 0) layerCount = image->createInfo.arrayLayers;
    if (levelCount == 0) levelCount = image->createInfo.mipLevels;
    // clear image
    PrepareForTransferDst(image, baseLayer, layerCount, mipLevel, levelCount);
    VkImageSubresourceRange range = {};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseArrayLayer = baseLayer;
    range.layerCount = layerCount;
    range.baseMipLevel = baseLayer;
    range.levelCount = levelCount;
    DeviceDispatch(vkCmdClearColorImage(handle, *image, image->layouts[baseLayer][mipLevel], &clear, 1, &range));
}

void CommandBuffer::ClearDepthStencil(Image* image,
                                      const VkClearDepthStencilValue& clear,
                                      uint32_t baseLayer, uint32_t layerCount,
                                      uint32_t mipLevel, uint32_t levelCount) {
    if (layerCount == 0) layerCount = image->createInfo.arrayLayers;
    if (levelCount == 0) levelCount = image->createInfo.mipLevels;
    PrepareForTransferDst(image, baseLayer, layerCount, mipLevel, levelCount);
    // clear image
    VkImageSubresourceRange range = {};
    range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    range.baseArrayLayer = baseLayer;
    range.layerCount = layerCount;
    range.baseMipLevel = baseLayer;
    range.levelCount = levelCount;
    DeviceDispatch(vkCmdClearDepthStencilImage(handle, *image, image->layouts[baseLayer][layerCount], &clear, 1, &range));
}

void CommandBuffer::SaveImage(const std::string& name, Image* image, uint32_t arrayLayer, uint32_t mipLevel) {
    const VkExtent3D& extent = image->GetExtent();
    const VkFormat format = image->GetFormat();

    if (extent.depth != 1) {
        throw std::runtime_error("[SaveImage] cannot handle 3d image saving yet!");
    }

    if (arrayLayer >= image->Layers()) {
        throw std::runtime_error("[SaveImage] target array layer >= max layers");
    }

    if (mipLevel >= image->MipLevels()) {
        throw std::runtime_error("[SaveImage] target mip level >= mip levels");
    }

    bool hdr = false;
    size_t size = 0;
    size_t channels = 0;
    switch (format) {
        case VK_FORMAT_R32_SFLOAT:          size = sizeof(float);   channels = 1; hdr = true;  break;
        case VK_FORMAT_R32G32_SFLOAT:       size = sizeof(float);   channels = 2; hdr = true;  break;
        case VK_FORMAT_R32G32B32_SFLOAT:    size = sizeof(float);   channels = 3; hdr = true;  break;
        case VK_FORMAT_R32G32B32A32_SFLOAT: size = sizeof(float);   channels = 4; hdr = true;  break;

        case VK_FORMAT_R8_UNORM:            size = sizeof(uint8_t); channels = 1; hdr = false; break;
        case VK_FORMAT_R8G8_UNORM:          size = sizeof(uint8_t); channels = 2; hdr = false; break;
        case VK_FORMAT_R8G8B8_UNORM:        size = sizeof(uint8_t); channels = 3; hdr = false; break;
        case VK_FORMAT_R8G8B8A8_UNORM:      size = sizeof(uint8_t); channels = 4; hdr = false; break;

        default: throw std::runtime_error("[SaveImage] unhandled image format for save!");
    }

    uint32_t w = std::max(1U, extent.width >> mipLevel);
    uint32_t h = std::max(1U, extent.height >> mipLevel);

    size_t bufsize = size * channels * w * h;
    auto buf = SlimPtr<StagingBuffer>(device, bufsize);
    stagingBuffers.emplace_back(buf);

    VkOffset3D off = VkOffset3D { 0, 0, 0 };
    VkExtent3D ext = VkExtent3D { w, h, 1 };
    CopyImageToBuffer(image, off, ext, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT, buf, 0, 0, 0);

    if (hdr) {
        stbi_write_hdr(name.c_str(), w, h, 4, buf->GetData<float>());
    } else {
        stbi_write_png(name.c_str(), w, h, 4, buf->GetData<char>(), w * size * channels);
    }
}
