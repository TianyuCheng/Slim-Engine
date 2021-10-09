#include <imgui.h>
#include "core/debug.h"
#include "core/vkutils.h"
#include "utility/rendergraph.h"

using namespace slim;

RenderGraph::Resource::Resource(Buffer* buffer)
    : buffer(buffer), retained(true) {
}

RenderGraph::Resource::Resource(GPUImage* image)
    : image(image), retained(true) {
    VkExtent3D ext = image->GetExtent();
    format = image->GetFormat();
    extent = { ext.width, ext.height };
    samples = image->GetSamples();
}

RenderGraph::Resource::Resource(VkFormat format, VkExtent2D extent, VkSampleCountFlagBits samples)
    : format(format), extent(extent), samples(samples), retained(false) {
}

void RenderGraph::Resource::Allocate(RenderFrame* renderFrame) {
    if (!image.get() && !buffer) {
        image = renderFrame->RequestGPUImage(format, extent, mipLevels, 1, samples, usages);
    }
}

void RenderGraph::Resource::Deallocate() {
    image.Reset();
}

void RenderGraph::Resource::ShaderReadBarrier(CommandBuffer* commandBuffer, RenderGraph::Pass* nextPass) {
    image->layouts[0][0] = currentLayout;

    // prepare layout transition
    VkImageLayout srcLayout = image->layouts[0][0];
    VkImageLayout dstLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // prepare src stages
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_NONE_KHR;
    if (!currentPass) {
        srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    } else {
        if (currentPass->IsCompute()) {
        srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        } else {
            srcStageMask = IsDepthStencil(format)
                         ? VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
                         : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }
    }

    // prepare dst stages
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_NONE_KHR;
    if (nextPass->IsCompute()) {
        dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    } else {
        dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    }

    // transit dst image layout
    PrepareLayoutTransition(commandBuffer,
        image,
        srcLayout, dstLayout,
        srcStageMask, dstStageMask,
        0, image->Layers(),
        0, image->MipLevels());

    // update next pass
    currentPass = nextPass;
    currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void RenderGraph::Resource::StorageBarrier(CommandBuffer* commandBuffer, RenderGraph::Pass* nextPass) {
    if (buffer) {
        StorageBufferBarrier(commandBuffer, nextPass);
    } else {
        StorageImageBarrier(commandBuffer, nextPass);
    }
}

void RenderGraph::Resource::StorageImageBarrier(CommandBuffer* commandBuffer, RenderGraph::Pass* nextPass) {
    image->layouts[0][0] = currentLayout;

    // prepare layout transition
    VkImageLayout srcLayout = image->layouts[0][0];
    VkImageLayout dstLayout = VK_IMAGE_LAYOUT_GENERAL;

    // prepare src stages
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_NONE_KHR;
    if (!currentPass) {
        srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    } else if (currentPass->IsCompute()) {
        srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    } else {
        srcStageMask = IsDepthStencil(format)
                     ? VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
                     : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }

    // prepare dst stages
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_NONE_KHR;
    if (nextPass->IsCompute()) {
        dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    } else {
        dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    }

    // transit dst image layout
    PrepareLayoutTransition(commandBuffer,
        image,
        srcLayout, dstLayout,
        srcStageMask, dstStageMask,
        0, image->Layers(),
        0, image->MipLevels());

    // update next pass
    currentPass = nextPass;
    currentLayout = VK_IMAGE_LAYOUT_GENERAL;
}

void RenderGraph::Resource::StorageBufferBarrier(CommandBuffer* commandBuffer, RenderGraph::Pass* nextPass) {
    Device* device = commandBuffer->GetDevice();

    // prepare src stages
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_NONE_KHR;
    if (!currentPass) {
        srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    } else if (currentPass->IsCompute()) {
        srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    } else {
        srcStageMask = IsDepthStencil(format)
                     ? VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
                     : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }

    // prepare dst stages
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_NONE_KHR;
    if (nextPass->IsCompute()) {
        dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    } else {
        dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    }

    VkBufferMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = *buffer;
    barrier.offset = 0;
    barrier.size = buffer->Size();
    DeviceDispatch(vkCmdPipelineBarrier(
        *commandBuffer,
        srcStageMask, dstStageMask,
        0,
        0, nullptr,  // memory barriers
        1, &barrier, // buffer memory barriers
        0, nullptr   // image memory barriers
    ));
}

RenderGraph::Subpass::Subpass(RenderGraph::Pass* parent) : parent(parent) {
}

void RenderGraph::Subpass::SetColorResolve(RenderGraph::Resource* resource) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::ColorResolveAttachment });
    resource->writers.push_back(parent);
    usedAsColorResolveAttachment.push_back(attachmentId);
    resource->UseAsColorBuffer();
}

void RenderGraph::Subpass::SetPreserve(RenderGraph::Resource* resource) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::PreserveAttachment });
    resource->readers.push_back(parent);
    resource->writers.push_back(parent);
    usedAsPreserveAttachment.push_back(attachmentId);
}

void RenderGraph::Subpass::SetInput(RenderGraph::Resource* resource) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::InputAttachment });
    resource->readers.push_back(parent);
    usedAsInputAttachment.push_back(attachmentId);
    resource->UseAsInputAttachment();
}

void RenderGraph::Subpass::SetColor(RenderGraph::Resource* resource) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::ColorAttachment });
    resource->writers.push_back(parent);
    usedAsColorAttachment.push_back(attachmentId);
    resource->UseAsColorBuffer();
}

void RenderGraph::Subpass::SetColor(RenderGraph::Resource* resource, const ClearValue& clear) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::ColorAttachment, clear });
    resource->writers.push_back(parent);
    usedAsColorAttachment.push_back(attachmentId);
    resource->UseAsColorBuffer();
}

void RenderGraph::Subpass::SetDepth(RenderGraph::Resource* resource) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::DepthAttachment });
    resource->writers.push_back(parent);
    usedAsDepthAttachment.push_back(attachmentId);
    resource->UseAsDepthBuffer();
}

void RenderGraph::Subpass::SetDepth(RenderGraph::Resource* resource, const ClearValue& clear) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::DepthAttachment, clear });
    resource->writers.push_back(parent);
    usedAsDepthAttachment.push_back(attachmentId);
    resource->UseAsDepthBuffer();
}

void RenderGraph::Subpass::SetStencil(RenderGraph::Resource* resource) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::StencilAttachment });
    resource->writers.push_back(parent);
    usedAsStencilAttachment.push_back(attachmentId);
    resource->UseAsStencilBuffer();
}

void RenderGraph::Subpass::SetStencil(RenderGraph::Resource* resource, const ClearValue& clear) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::StencilAttachment, clear });
    resource->writers.push_back(parent);
    usedAsStencilAttachment.push_back(attachmentId);
    resource->UseAsStencilBuffer();
}

void RenderGraph::Subpass::SetDepthStencil(RenderGraph::Resource* resource) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::DepthStencilAttachment });
    resource->writers.push_back(parent);
    usedAsDepthStencilAttachment.push_back(attachmentId);
    resource->UseAsDepthStencilBuffer();
}

void RenderGraph::Subpass::SetDepthStencil(RenderGraph::Resource* resource, const ClearValue& clear) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::DepthStencilAttachment, clear });
    resource->writers.push_back(parent);
    usedAsDepthStencilAttachment.push_back(attachmentId);
    resource->UseAsDepthStencilBuffer();
}

void RenderGraph::Subpass::SetTexture(RenderGraph::Resource* resource) {
    uint32_t textureId = parent->AddTexture(resource);
    resource->readers.push_back(parent);
    usedAsTexture.push_back(textureId);
    resource->UseAsTexture();
}

void RenderGraph::Subpass::SetStorage(RenderGraph::Resource* resource, uint32_t usage) {
    uint32_t storageId = parent->AddStorage(resource);
    // a storage resource could both be read and written
    if (usage & STORAGE_READ_BIT)  resource->readers.push_back(parent);
    if (usage & STORAGE_WRITE_BIT) resource->writers.push_back(parent);

    if (resource->GetBuffer()) {
        usedAsStorageBuffer.push_back(storageId);
    } else {
        usedAsStorageImage.push_back(storageId);
        resource->UseAsStorageImage();
    }
}

void RenderGraph::Subpass::Execute(std::function<void(const RenderInfo& renderInfo)> callback) {
    this->callback = callback;
}

RenderGraph::Pass::Pass(const std::string& name, RenderGraph* graph, bool compute) : name(name), graph(graph), compute(compute) {
    defaultSubpass = SlimPtr<Subpass>(this);
}

RenderGraph::Subpass* RenderGraph::Pass::CreateSubpass() {
    assert(!compute && "ComputePass does not support subpass");
    subpasses.push_back(SlimPtr<RenderGraph::Subpass>(this));
    useDefaultSubpass = false;
    return subpasses.back();
}

void RenderGraph::Pass::SetColorResolve(RenderGraph::Resource* resource) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetColorResolve(resource);
}

void RenderGraph::Pass::SetPreserve(RenderGraph::Resource* resource) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetPreserve(resource);
}

void RenderGraph::Pass::SetColor(RenderGraph::Resource* resource) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetColor(resource);
}

void RenderGraph::Pass::SetColor(RenderGraph::Resource* resource, const ClearValue& clear) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetColor(resource, clear);
}

void RenderGraph::Pass::SetDepth(RenderGraph::Resource* resource) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetDepth(resource);
}

void RenderGraph::Pass::SetDepth(RenderGraph::Resource* resource, const ClearValue& clear) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetDepth(resource, clear);
}

void RenderGraph::Pass::SetStencil(RenderGraph::Resource* resource) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetStencil(resource);
}

void RenderGraph::Pass::SetStencil(RenderGraph::Resource* resource, const ClearValue& clear) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetStencil(resource, clear);
}

void RenderGraph::Pass::SetDepthStencil(RenderGraph::Resource* resource) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetDepthStencil(resource);
}

void RenderGraph::Pass::SetDepthStencil(RenderGraph::Resource* resource, const ClearValue& clear) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetDepthStencil(resource, clear);
}

void RenderGraph::Pass::SetTexture(RenderGraph::Resource* resource) {
    defaultSubpass->SetTexture(resource);
}

void RenderGraph::Pass::SetStorage(RenderGraph::Resource* resource, uint32_t usage) {
    defaultSubpass->SetStorage(resource, usage);
}

uint32_t RenderGraph::Pass::AddAttachment(const RenderGraph::ResourceMetadata& metadata) {
    // find the resource
    auto it = attachmentMap.find(metadata.resource);
    if (it == attachmentMap.end()) {
        uint32_t attachmentId = attachments.size();
        // adding to attachments
        attachments.push_back(metadata);
        // update attachment mapping
        attachmentMap.insert(std::make_pair(metadata.resource, attachmentId));
        return attachmentId;
    }
    // update clear value
    attachments[it->second].clearValue = metadata.clearValue;
    return it->second;
}

uint32_t RenderGraph::Pass::AddTexture(Resource* resource) {
    // find the resource
    auto it = textureMap.find(resource);
    if (it == textureMap.end()) {
        uint32_t textureId = textures.size();
        // adding to textures
        textures.push_back(resource);
        // update texture mapping
        textureMap.insert(std::make_pair(resource, textureId));
        return textureId;
    }
    return it->second;
}

uint32_t RenderGraph::Pass::AddStorage(Resource* resource) {
    // find the resource
    auto it = storageMap.find(resource);
    if (it == storageMap.end()) {
        uint32_t storageId = storages.size();
        // adding to storages
        storages.push_back(resource);
        // update storage mapping
        storageMap.insert(std::make_pair(resource, storageId));
        return storageId;
    }
    return it->second;
}

void RenderGraph::Pass::Execute(std::function<void(const RenderInfo &renderInfo)> callback) {
    assert(useDefaultSubpass && "call subpass's Execute function when not using the default subpass");
    defaultSubpass->Execute(callback);
}

void RenderGraph::Pass::Execute(CommandBuffer* commandBuffer) {
    RenderFrame* renderFrame = graph->GetRenderFrame();

    // allocate attachment resource if necessary
    for (auto& attachment : attachments) {
        attachment.resource->Allocate(renderFrame);
    }

    // allocate storage images if necessary
    for (auto& storage : storages) {
        storage->Allocate(renderFrame);
    }

    // wait for texture resources
    for (auto& texture : textures) {
        texture->ShaderReadBarrier(commandBuffer, this);
    }

    // wait for storage resources
    for (auto& storage : storages) {
        storage->StorageBarrier(commandBuffer, this);
    }

    commandBuffer->BeginRegion(name);
    if (compute) {
        ExecuteCompute(commandBuffer);
    } else {
        ExecuteGraphics(commandBuffer);
    }
    commandBuffer->EndRegion();

    // update reference counts for attachments
    for (auto& attachment : attachments) {
        attachment.resource->wrCount--;
        uint32_t refCount = attachment.resource->rdCount
                          + attachment.resource->wrCount;
        if (refCount == 0) {
            attachment.resource->Deallocate();
        }
    }

    // update reference counts for textures
    for (auto* texture : textures) {
        texture->rdCount--;
        uint32_t refCount = texture->rdCount
                          + texture->wrCount;
        if (refCount == 0) {
            texture->Deallocate();
        }
    }
}

void RenderGraph::Pass::ExecuteCompute(CommandBuffer* commandBuffer) {
    assert(useDefaultSubpass && "ComputePass only support the default subpass");

    // execute compute callback
    RenderInfo info;
    info.renderGraph = graph;
    info.renderFrame = graph->GetRenderFrame();
    info.renderPass = nullptr;
    info.commandBuffer = commandBuffer;
    defaultSubpass->callback(info);
}

void RenderGraph::Pass::ExecuteGraphics(CommandBuffer* commandBuffer) {
    // prepare a render pass
    RenderPassDesc renderPassDesc;
    FramebufferDesc framebufferDesc;
    std::vector<VkClearValue> clearValues;

    auto inferLoadOp = [](const ResourceMetadata &attachment) -> VkAttachmentLoadOp {
        if (attachment.clearValue.has_value()) {
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        } if (attachment.resource->currentLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    };

    std::vector<uint32_t> attachmentIds = {};
    for (const auto& attachment : attachments) {
        Resource* resource = attachment.resource;
        VkAttachmentLoadOp load = inferLoadOp(attachment);
        VkAttachmentStoreOp store = VK_ATTACHMENT_STORE_OP_STORE;
        uint32_t attachmentId = 0;
        switch (attachment.type) {
            case ResourceType::ColorAttachment:
                attachmentId = renderPassDesc.AddAttachment(resource->format, resource->samples, load, store, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE);
                framebufferDesc.AddAttachment(attachment.resource->image->AsColorBuffer());
                break;
            case ResourceType::ColorResolveAttachment:
                attachmentId = renderPassDesc.AddAttachment(resource->format, resource->samples, load, store, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE);
                framebufferDesc.AddAttachment(attachment.resource->image->AsColorBuffer());
                break;
            case ResourceType::DepthAttachment:
                attachmentId = renderPassDesc.AddAttachment(resource->format, resource->samples, load, store, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE);
                framebufferDesc.AddAttachment(attachment.resource->image->AsDepthBuffer());
                break;
            case ResourceType::StencilAttachment:
                attachmentId = renderPassDesc.AddAttachment(resource->format, resource->samples, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, load, store);
                framebufferDesc.AddAttachment(attachment.resource->image->AsStencilBuffer());
                break;
            case ResourceType::DepthStencilAttachment:
                attachmentId = renderPassDesc.AddAttachment(resource->format, resource->samples, load, store, load, store);
                framebufferDesc.AddAttachment(attachment.resource->image->AsDepthStencilBuffer());
                break;
            case ResourceType::PreserveAttachment:
                // preserve attachment does not need to be specified in the framebuffer creation
                break;
            case ResourceType::InputAttachment:
                // input attachment does not need to be specified in the framebuffer creation
                break;
            case ResourceType::StorageBuffer:
                // input attachment does not need to be specified in the framebuffer creation
                break;
        }
        if (attachment.clearValue.has_value()) {
            clearValues.push_back(attachment.clearValue.value());
        } else {
            clearValues.push_back(ClearValue(1.0, 1.0, 1.0, 1.0));
        }
        attachmentIds.push_back(attachmentId);
    }

    // update render pass subpasses
    std::vector<SmartPtr<Subpass>> renderSubpasses;
    if (useDefaultSubpass) {
        renderSubpasses.push_back(defaultSubpass);
    } else {
        renderSubpasses = subpasses;
    }
    for (const auto& subpass : renderSubpasses) {
        SubpassDesc& subpassDesc = renderPassDesc.AddSubpass();
        for (uint32_t attachment : subpass->usedAsColorAttachment) {
            uint32_t attachmentId = attachmentIds[attachment];
            VkImageLayout initialLayout = attachments[attachment].resource->currentLayout;
            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            subpassDesc.AddColorAttachment(attachmentId, initialLayout, finalLayout);
            attachments[attachment].resource->currentLayout = finalLayout;
        }
        for (uint32_t attachment : subpass->usedAsColorResolveAttachment) {
            uint32_t attachmentId = attachmentIds[attachment];
            VkImageLayout initialLayout = attachments[attachment].resource->currentLayout;
            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            subpassDesc.AddResolveAttachment(attachmentId, initialLayout, finalLayout);
            attachments[attachment].resource->currentLayout = finalLayout;
        }
        for (uint32_t attachment : subpass->usedAsDepthAttachment) {
            uint32_t attachmentId = attachmentIds[attachment];
            VkImageLayout initialLayout = attachments[attachment].resource->currentLayout;
            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            subpassDesc.AddDepthAttachment(attachmentId, initialLayout, finalLayout);
            attachments[attachment].resource->currentLayout = finalLayout;
        }
        for (uint32_t attachment : subpass->usedAsStencilAttachment) {
            uint32_t attachmentId = attachmentIds[attachment];
            VkImageLayout initialLayout = attachments[attachment].resource->currentLayout;
            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            subpassDesc.AddStencilAttachment(attachmentId, initialLayout, finalLayout);
            attachments[attachment].resource->currentLayout = finalLayout;
        }
        for (uint32_t attachment : subpass->usedAsDepthStencilAttachment) {
            uint32_t attachmentId = attachmentIds[attachment];
            VkImageLayout initialLayout = attachments[attachment].resource->currentLayout;
            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            subpassDesc.AddDepthStencilAttachment(attachmentId, initialLayout, finalLayout);
            attachments[attachment].resource->currentLayout = finalLayout;
        }
        for (uint32_t attachment : subpass->usedAsInputAttachment) {
            uint32_t attachmentId = attachmentIds[attachment];
            VkImageLayout initialLayout = attachments[attachment].resource->currentLayout;
            VkImageLayout finalLayout = attachments[attachment].resource->currentLayout;
            subpassDesc.AddInputAttachment(attachmentId, initialLayout, finalLayout);
            attachments[attachment].resource->currentLayout = finalLayout;
        }
        for (uint32_t attachment : subpass->usedAsPreserveAttachment) {
            uint32_t attachmentId = attachmentIds[attachment];
            subpassDesc.AddPreserveAttachment(attachmentId);
        }
    }

    // framebuffer extent
    assert(attachments.size() > 0);
    VkExtent2D extent = attachments[0].resource->extent;
    framebufferDesc.SetExtent(extent.width, extent.height);

    // use a named render pass desc
    renderPassDesc.SetName(name);

    RenderFrame* renderFrame = graph->GetRenderFrame();
    RenderPass* renderPass = renderFrame->RequestRenderPass(renderPassDesc);
    Framebuffer* framebuffer = renderFrame->RequestFramebuffer(framebufferDesc.SetRenderPass(renderPass));
    Device* device = renderFrame->GetDevice();

    // update render pass
    graph->renderPass.reset(renderPass);

    // begin render pass
    VkRenderPassBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.framebuffer = *framebuffer;
    beginInfo.renderPass = *renderPass;
    beginInfo.renderArea.offset = { 0, 0 };
    beginInfo.renderArea.extent = extent;
    beginInfo.clearValueCount = clearValues.size();
    beginInfo.pClearValues = clearValues.data();
    beginInfo.pNext = nullptr;
    commandBuffer->BeginRenderPass(beginInfo);

    RenderInfo info;
    info.renderGraph = graph;
    info.renderFrame = renderFrame;
    info.renderPass = renderPass;
    info.commandBuffer = commandBuffer;

    // execute draw callback
    for (uint32_t i = 0; i < renderSubpasses.size(); i++) {
        renderSubpasses[i]->callback(info);
        if (i != renderSubpasses.size() - 1) {
            commandBuffer->NextSubpass();
        }
    }

    // end render pass
    commandBuffer->EndRenderPass();
}

RenderGraph::RenderGraph(RenderFrame *frame)
    : renderFrame(frame) {

}

RenderGraph::~RenderGraph() {

}

RenderFrame* RenderGraph::GetRenderFrame() const {
    return renderFrame.get();
}

RenderPass* RenderGraph::GetRenderPass() const {
    return renderPass.get();
}

CommandBuffer* RenderGraph::GetCommandBuffer() const {
    return commandBuffer.get();
}

RenderGraph::Pass* RenderGraph::CreateRenderPass(const std::string& name) {
    passes.push_back(SlimPtr<Pass>(name, this, false));
    return passes.back().get();
}

RenderGraph::Pass* RenderGraph::CreateComputePass(const std::string& name) {
    passes.push_back(SlimPtr<Pass>(name, this, true));
    return passes.back().get();
}

RenderGraph::Resource* RenderGraph::CreateResource(Buffer* buffer) {
    resources.push_back(SlimPtr<Resource>(buffer));
    return resources.back().get();
}

RenderGraph::Resource* RenderGraph::CreateResource(GPUImage* image) {
    resources.push_back(SlimPtr<Resource>(image));
    return resources.back().get();
}

RenderGraph::Resource* RenderGraph::CreateResource(VkExtent2D extent, VkFormat format, VkSampleCountFlagBits samples) {
    resources.push_back(SlimPtr<Resource>(format, extent, samples));
    return resources.back().get();
}

void RenderGraph::Compile() {
    // initialize resource status
    for (auto& resource : resources) {
        resource->rdCount = resource->readers.size();
        resource->wrCount = resource->writers.size();
    }

    // initialize pass status
    for (auto& pass : passes) {
        pass->visited = false;
    }

    // for each retained resource, find a path back
    // there must be at least 1 resource that is retained (for back buffer)
    for (auto& resource : resources) {
        if (resource->retained) {
            CompileResource(resource.get());
        }
    }

    // mark compilation completion
    compiled = true;
}

void RenderGraph::CompileResource(Resource* resource) {
    // passes that write to this resource must be retained
    for (Pass* pass : resource->writers) {
        CompilePass(pass);
    }
}

void RenderGraph::CompilePass(Pass* pass) {
    // in case this pass is already processed, we don't process it anymore
    if (pass->retained) return;

    // mark this pass as retained to avoid repetitive addition of this pass
    pass->retained = true;

    // any texture used by this pass should be retained
    for (auto &texture : pass->textures) CompileResource(texture);
    for (auto &storage : pass->storages) CompileResource(storage);
}

std::unordered_set<RenderGraph::Pass*> RenderGraph::FindPassDependencies(Pass* pass) {
    std::unordered_set<RenderGraph::Pass*> dependencies;

    // textures
    for (const auto& texture : pass->textures)
        for (const auto& writerPass : texture->writers)
            dependencies.insert(writerPass);

    // storages
    for (const auto& storage : pass->storages)
        for (const auto& writerPass : storage->writers)
            dependencies.insert(writerPass);

    // remove self dependency
    dependencies.erase(pass);

    return dependencies;
}

void RenderGraph::Execute() {
    if (!compiled) Compile();

    // NOTE: Technically we will need a compute buffer for each compute pass.
    // For dependency synchronization, any draw pass that needs input from compute pass
    // would require separate command buffer;

    CommandBuffer* commandBuffer = renderFrame->RequestCommandBuffer(VK_QUEUE_GRAPHICS_BIT);
    commandBuffer->Begin();
    commandBuffer->BeginRegion("RenderGraph");

    // execute
    for (auto& pass : passes) {
        // don't execute culled passes
        if (!pass->retained) return;

        // execute the pass
        pass->Execute(commandBuffer);
        pass->visited = true;
    }

    // present src layout transition
    GPUImage* backBuffer = renderFrame->GetBackBuffer();
    if (backBuffer) {
        VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        for (const auto& resource : resources) {
            if (resource->image.get() == backBuffer) {
                layout = resource->currentLayout;
                break;
            }
        }
        backBuffer->layouts[0][0] = layout;
        if (renderFrame->Presentable()) {
            commandBuffer->PrepareForPresentSrc(backBuffer);
        }
    }

    // stop command buffer recording
    commandBuffer->EndRegion();
    commandBuffer->End();

    // submit the last graphics command buffer for presentation
    if (renderFrame->Presentable()) {
        renderFrame->Present(commandBuffer);
    } else {
        renderFrame->Draw(commandBuffer);
    }
}

void RenderGraph::Visualize() {
    if (!compiled) Compile();
    // dothing for now
}

void RenderGraph::Print() {
    if (!compiled) Compile();
    // dothing for now
}
