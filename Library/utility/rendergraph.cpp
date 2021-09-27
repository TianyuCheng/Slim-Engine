#include <numeric>
#include <imgui.h>
#include "core/debug.h"
#include "core/vkutils.h"
#include "utility/rendergraph.h"

#define SLIM_DEBUG_RENDERGRAPH 0

using namespace slim;

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
    if (!image.get()) {
        image = renderFrame->RequestGPUImage(format, extent, mipLevels, 1, samples, usages);
    }
}

void RenderGraph::Resource::Deallocate() {
    image.Reset();
}

RenderGraph::Subpass::Subpass(RenderGraph::Pass* parent) : parent(parent) {
}

void RenderGraph::Subpass::SetColorResolve(RenderGraph::Resource *resource) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::ColorResolveAttachment });
    resource->writers.push_back(parent);
    usedAsColorResolveAttachment.push_back(attachmentId);
    resource->UseAsColorBuffer();
}

void RenderGraph::Subpass::SetPreserve(RenderGraph::Resource *resource) {
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

void RenderGraph::Subpass::SetColor(RenderGraph::Resource *resource) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::ColorAttachment });
    resource->writers.push_back(parent);
    usedAsColorAttachment.push_back(attachmentId);
    resource->UseAsColorBuffer();
}

void RenderGraph::Subpass::SetColor(RenderGraph::Resource *resource, const ClearValue &clear) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::ColorAttachment, clear });
    resource->writers.push_back(parent);
    usedAsColorAttachment.push_back(attachmentId);
    resource->UseAsColorBuffer();
}

void RenderGraph::Subpass::SetDepth(RenderGraph::Resource *resource) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::DepthAttachment });
    resource->writers.push_back(parent);
    usedAsDepthAttachment.push_back(attachmentId);
    resource->UseAsDepthBuffer();
}

void RenderGraph::Subpass::SetDepth(RenderGraph::Resource *resource, const ClearValue &clear) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::DepthAttachment, clear });
    resource->writers.push_back(parent);
    usedAsDepthAttachment.push_back(attachmentId);
    resource->UseAsDepthBuffer();
}

void RenderGraph::Subpass::SetStencil(RenderGraph::Resource *resource) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::StencilAttachment });
    resource->writers.push_back(parent);
    usedAsStencilAttachment.push_back(attachmentId);
    resource->UseAsStencilBuffer();
}

void RenderGraph::Subpass::SetStencil(RenderGraph::Resource *resource, const ClearValue &clear) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::StencilAttachment, clear });
    resource->writers.push_back(parent);
    usedAsStencilAttachment.push_back(attachmentId);
    resource->UseAsStencilBuffer();
}

void RenderGraph::Subpass::SetDepthStencil(RenderGraph::Resource *resource) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::DepthStencilAttachment });
    resource->writers.push_back(parent);
    usedAsDepthStencilAttachment.push_back(attachmentId);
    resource->UseAsDepthStencilBuffer();
}

void RenderGraph::Subpass::SetDepthStencil(RenderGraph::Resource *resource, const ClearValue &clear) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::DepthStencilAttachment, clear });
    resource->writers.push_back(parent);
    usedAsDepthStencilAttachment.push_back(attachmentId);
    resource->UseAsDepthStencilBuffer();
}

void RenderGraph::Subpass::SetTexture(RenderGraph::Resource *resource) {
    uint32_t textureId = parent->AddTexture(resource);
    resource->readers.push_back(parent);
    usedAsTexture.push_back(textureId);
    resource->UseAsTexture();
}

void RenderGraph::Subpass::SetStorage(RenderGraph::Resource *resource, uint32_t usage) {
    uint32_t storageId = parent->AddStorage(resource);
    // a storage resource could both be read and written
    if (usage & STORAGE_IMAGE_READ_BIT)  resource->readers.push_back(parent);
    if (usage & STORAGE_IMAGE_WRITE_BIT) resource->writers.push_back(parent);
    usedAsStorage.push_back(storageId);
    resource->UseAsStorage();
}

void RenderGraph::Subpass::Execute(std::function<void(const RenderInfo &renderInfo)> callback) {
    this->callback = callback;
}

RenderGraph::Pass::Pass(const std::string &name, RenderGraph *graph, bool compute) : name(name), graph(graph), compute(compute) {
    defaultSubpass = SlimPtr<Subpass>(this);
}

RenderGraph::Subpass* RenderGraph::Pass::CreateSubpass() {
    assert(!compute && "ComputePass does not support subpass");
    subpasses.push_back(SlimPtr<RenderGraph::Subpass>(this));
    useDefaultSubpass = false;
    return subpasses.back();
}

void RenderGraph::Pass::SetColorResolve(RenderGraph::Resource *resource) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetColorResolve(resource);
}

void RenderGraph::Pass::SetPreserve(RenderGraph::Resource *resource) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetPreserve(resource);
}

void RenderGraph::Pass::SetColor(RenderGraph::Resource *resource) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetColor(resource);
}

void RenderGraph::Pass::SetColor(RenderGraph::Resource *resource, const ClearValue &clear) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetColor(resource, clear);
}

void RenderGraph::Pass::SetDepth(RenderGraph::Resource *resource) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetDepth(resource);
}

void RenderGraph::Pass::SetDepth(RenderGraph::Resource *resource, const ClearValue &clear) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetDepth(resource, clear);
}

void RenderGraph::Pass::SetStencil(RenderGraph::Resource *resource) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetStencil(resource);
}

void RenderGraph::Pass::SetStencil(RenderGraph::Resource *resource, const ClearValue &clear) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetStencil(resource, clear);
}

void RenderGraph::Pass::SetDepthStencil(RenderGraph::Resource *resource) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetDepthStencil(resource);
}

void RenderGraph::Pass::SetDepthStencil(RenderGraph::Resource *resource, const ClearValue &clear) {
    assert(useDefaultSubpass && "call subpass's Set* function when not using the default subpass");
    defaultSubpass->SetDepthStencil(resource, clear);
}

void RenderGraph::Pass::SetTexture(RenderGraph::Resource *resource) {
    defaultSubpass->SetTexture(resource);
}

void RenderGraph::Pass::SetStorage(RenderGraph::Resource *resource, uint32_t usage) {
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
    #ifndef NDEBUG
    if (!compute) {
        throw std::runtime_error("AddStorage should only be used for compute passes");
    }
    #endif

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
    for (auto& storage : storages) {
        storage->Allocate(renderFrame);
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

void RenderGraph::Pass::TransitTextureLayout(RenderGraph::Resource* resource) {
    resource->image->layouts[0][0] = resource->layout;
    resource->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkImageLayout srcLayout = resource->image->layouts[0][0];
    VkImageLayout dstLayout = resource->layout;
    VkPipelineStageFlags srcStageMask = IsDepthStencil(resource->format)
                                      ? VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
                                      : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT; // missing the information which stage texture is used

    // transit dst image layout
    PrepareLayoutTransition(*commandBuffer,
        resource->image,
        srcLayout, dstLayout,
        srcStageMask, dstStageMask,
        0, resource->image->Layers(),
        0, resource->image->MipLevels());
}

void RenderGraph::Pass::TransitStorageLayout(RenderGraph::Resource* resource) {
    resource->image->layouts[0][0] = resource->layout;
    resource->layout = VK_IMAGE_LAYOUT_GENERAL;
    VkImageLayout srcLayout = resource->image->layouts[0][0];
    VkImageLayout dstLayout = resource->layout;
    VkPipelineStageFlags srcStageMask = IsDepthStencil(resource->format)
                                      ? VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
                                      : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT; // missing the information which stage storage is used

    // transit dst image layout
    PrepareLayoutTransition(*commandBuffer,
        resource->image,
        srcLayout, dstLayout,
        srcStageMask, dstStageMask,
        0, resource->image->Layers(),
        0, resource->image->MipLevels());
}

void RenderGraph::Pass::ExecuteCompute(CommandBuffer* commandBuffer) {
    assert(useDefaultSubpass && "ComputePass only support the default subpass");

    // texture layout transition
    for (auto &id : defaultSubpass->usedAsTexture) {
        TransitTextureLayout(textures[id]);
    }

    // storage layout transition
    for (auto &id : defaultSubpass->usedAsStorage) {
        TransitStorageLayout(storages[id]);
    }

    // TODO: add other types of resources which needs layout transition

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
        } if (attachment.resource->layout == VK_IMAGE_LAYOUT_UNDEFINED) {
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
                // attachmentId = renderPassDesc.AddAttachment(resource->format, resource->samples, load, store, load, store);
                break;
            case ResourceType::InputAttachment:
                // input attachment does not need to be specified in the framebuffer creation
                // attachmentId = renderPassDesc.AddAttachment(resource->format, resource->samples, load, store, load, store);
                break;
        }
        if (attachment.clearValue.has_value()) {
            clearValues.push_back(attachment.clearValue.value());
        } else {
            clearValues.push_back(ClearValue(1.0, 1.0, 1.0, 1.0));
        }
        attachmentIds.push_back(attachmentId);
    }

    // texture layout transition
    for (auto &id : defaultSubpass->usedAsTexture) {
        TransitTextureLayout(textures[id]);
    }

    // storage layout transition
    for (auto &id : defaultSubpass->usedAsStorage) {
        TransitStorageLayout(storages[id]);
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
            VkImageLayout initialLayout = attachments[attachment].resource->layout;
            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            subpassDesc.AddColorAttachment(attachmentId, initialLayout, finalLayout);
            attachments[attachment].resource->layout = finalLayout;
        }
        for (uint32_t attachment : subpass->usedAsColorResolveAttachment) {
            uint32_t attachmentId = attachmentIds[attachment];
            VkImageLayout initialLayout = attachments[attachment].resource->layout;
            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            subpassDesc.AddResolveAttachment(attachmentId, initialLayout, finalLayout);
            attachments[attachment].resource->layout = finalLayout;
        }
        for (uint32_t attachment : subpass->usedAsDepthAttachment) {
            uint32_t attachmentId = attachmentIds[attachment];
            VkImageLayout initialLayout = attachments[attachment].resource->layout;
            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            subpassDesc.AddDepthAttachment(attachmentId, initialLayout, finalLayout);
            attachments[attachment].resource->layout = finalLayout;
        }
        for (uint32_t attachment : subpass->usedAsStencilAttachment) {
            uint32_t attachmentId = attachmentIds[attachment];
            VkImageLayout initialLayout = attachments[attachment].resource->layout;
            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            subpassDesc.AddStencilAttachment(attachmentId, initialLayout, finalLayout);
            attachments[attachment].resource->layout = finalLayout;
        }
        for (uint32_t attachment : subpass->usedAsDepthStencilAttachment) {
            uint32_t attachmentId = attachmentIds[attachment];
            VkImageLayout initialLayout = attachments[attachment].resource->layout;
            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            subpassDesc.AddDepthStencilAttachment(attachmentId, initialLayout, finalLayout);
            attachments[attachment].resource->layout = finalLayout;
        }
        for (uint32_t attachment : subpass->usedAsInputAttachment) {
            uint32_t attachmentId = attachmentIds[attachment];
            VkImageLayout initialLayout = attachments[attachment].resource->layout;
            VkImageLayout finalLayout = attachments[attachment].resource->layout;
            subpassDesc.AddInputAttachment(attachmentId, initialLayout, finalLayout);
            attachments[attachment].resource->layout = finalLayout;
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
    DeviceDispatch(vkCmdBeginRenderPass(*commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE));

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
    DeviceDispatch(vkCmdEndRenderPass(*commandBuffer));
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

RenderGraph::Pass* RenderGraph::CreateRenderPass(const std::string &name) {
    passes.push_back(SlimPtr<Pass>(name, this, false));
    return passes.back().get();
}

RenderGraph::Pass* RenderGraph::CreateComputePass(const std::string &name) {
    passes.push_back(SlimPtr<Pass>(name, this, true));
    return passes.back().get();
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
    for (auto &resource : resources) {
        resource->rdCount = resource->readers.size();
        resource->wrCount = resource->writers.size();
    }

    // initialize pass status
    for (auto &pass : passes) {
        pass->visited = false;
    }

    // for each retained resource, find a path back
    // there must be at least 1 resource that is retained (for back buffer)
    for (auto &resource : resources) {
        if (resource->retained) {
            CompileResource(resource.get());
        }
    }

    // mark compilation completion
    compiled = true;
}

void RenderGraph::CompileResource(Resource *resource) {
    // passes that write to this resource must be retained
    for (Pass *pass : resource->writers) {
        CompilePass(pass);
    }
}

void RenderGraph::CompilePass(Pass *pass) {
    // in case this pass is already processed, we don't process it anymore
    if (pass->retained) return;

    // mark this pass as retained to avoid repetitive addition of this pass
    pass->retained = true;

    // any texture used by this pass should be retained
    for (auto &texture : pass->textures) CompileResource(texture);
    for (auto &storage : pass->storages) CompileResource(storage);

    // adding this pass in a post-order
    timeline.push_back(pass);
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

    CommandBuffer* lastGraphicsCommandBuffer = nullptr;
    std::vector<CommandBuffer*> uniqueCommandBuffers = {};

    #if SLIM_DEBUG_RENDERGRAPH
    for (Pass *pass : timeline) {
        auto dependencies = FindPassDependencies(pass);
        std::cout << "Pass [" << pass->name << "] depends on" << std::endl;
        for (auto& dep : dependencies) {
            std::cout << " - " << dep->name << std::endl;
        }
    }
    #endif

    for (Pass *pass : timeline) {
        // don't execute a pass if it is executed
        // theoretically it should have been taken care of by Compile step
        #ifndef NDEBUG
        assert(!pass->visited);
        assert(pass->retained);
        #endif

        // assume all dependencies should have been processed
        auto dependencies = FindPassDependencies(pass);

        // figure out semaphores to wait for
        std::unordered_set<Semaphore*> waitSemaphores = {};
        for (const auto& dep : dependencies) {
            #ifndef NDEBUG
            if (dep->signalSemaphore.get() == nullptr) {
                std::cerr << "Pass[" << pass->name << "] detects a dependency from pass["
                          << dep->name << "], but the dependency pass has a null signal semaphore"
                          << std::endl;
                throw std::runtime_error("[RenderGraph] internal error");
            }
            #endif
            waitSemaphores.insert(dep->signalSemaphore);
        }

        if (pass->compute) {
            // all compute passes use separate command buffer
            pass->commandBuffer = renderFrame->RequestCommandBuffer(VK_QUEUE_COMPUTE_BIT);
            pass->signalSemaphore = renderFrame->RequestSemaphore();
            pass->commandBuffer->Signal(pass->signalSemaphore);
            for (auto& semaphore : waitSemaphores) {
                pass->commandBuffer->Wait(semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
            }
            pass->commandBuffer->Begin();
            uniqueCommandBuffers.push_back(pass->commandBuffer);
        } else {
            bool newCommandBuffer = true;

            // special case:
            // when there is exactly one dependency, and the dependency is also a render pass,
            // use the same command buffer as the last one
            if (dependencies.size() == 1) {
                auto& dep = *dependencies.begin();
                if (!dep->compute) {
                    newCommandBuffer = false;
                    pass->commandBuffer = dep->commandBuffer;
                    pass->signalSemaphore = dep->signalSemaphore;
                }
            }

            // create a new command buffer
            if (newCommandBuffer) {
                pass->commandBuffer = renderFrame->RequestCommandBuffer(VK_QUEUE_COMPUTE_BIT);
                pass->signalSemaphore = renderFrame->RequestSemaphore();
                pass->commandBuffer->Signal(pass->signalSemaphore);
                for (auto& semaphore : waitSemaphores) {
                    pass->commandBuffer->Wait(semaphore, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
                }
                pass->commandBuffer->Begin();
                lastGraphicsCommandBuffer = pass->commandBuffer;
                uniqueCommandBuffers.push_back(pass->commandBuffer);
            }
        }

        // execute the pass
        pass->Execute(pass->commandBuffer);
        pass->visited = true;

        // debug info
        #if SLIM_DEBUG_RENDERGRAPH
        std::cout << "Pass: " << pass->name << std::endl;
        std::cout << "Command Buffer: " << *pass->commandBuffer << std::endl;
        std::cout << "Signal Semaphore: " << *pass->signalSemaphore << std::endl;
        std::cout << "Dependencies: " << std::endl;
        for (const auto& dep : dependencies) {
            std::cout << "- " << dep->name << std::endl;
        }
        std::cout << "--------------------------------" << std::endl;
        #endif
    }

    #if SLIM_DEBUG_RENDERGRAPH
    if (lastGraphicsCommandBuffer) {
        std::cout << "* last graphics command buffer: " << *lastGraphicsCommandBuffer << std::endl;
    }

    for (Pass *pass : timeline) {
        if (pass->commandBuffer != lastGraphicsCommandBuffer) {
            auto info = pass->commandBuffer->GetSubmitInfo();
            std::cout << "Pass Submit Info [Name] = " << pass->name << std::endl;
            std::cout << "Wait Semaphore Count: " << info.waitSemaphoreCount << std::endl;
            for (uint32_t i = 0; i < info.waitSemaphoreCount; i++) {
                std::cout << " - " << info.pWaitSemaphores[i] << std::endl;
            }
            std::cout << "Signal Semaphore Count: " << info.signalSemaphoreCount << std::endl;
            for (uint32_t i = 0; i < info.signalSemaphoreCount; i++) {
                std::cout << " - " << info.pSignalSemaphores[i] << std::endl;
            }
        }
    }
    #endif

    // adding a layout transition
    if (lastGraphicsCommandBuffer) {
        // present src layout transition
        GPUImage* backBuffer = renderFrame->GetBackBuffer();
        VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        for (const auto& resource : resources) {
            if (resource->image.get() == backBuffer) {
                layout = resource->layout;
                break;
            }
        }
        backBuffer->layouts[0][0] = layout;
        if (renderFrame->Presentable()) {
            lastGraphicsCommandBuffer->PrepareForPresentSrc(backBuffer);
        }
    }

    // stop command buffer recording for all
    for (auto& commandBuffer : uniqueCommandBuffers) {
        commandBuffer->End();
    }

    // submit all command buffers (except for the last)
    for (auto& commandBuffer : uniqueCommandBuffers) {
        if (commandBuffer != lastGraphicsCommandBuffer) {
            commandBuffer->Submit();
        }
    }

    // submit the last graphics command buffer for presentation
    if (lastGraphicsCommandBuffer) {
        if (renderFrame->Presentable()) {
            renderFrame->Present(lastGraphicsCommandBuffer);
        } else {
            renderFrame->Draw(lastGraphicsCommandBuffer);
        }
    }
}

void RenderGraph::Visualize() {
    if (!compiled) Compile();

    for (auto pass : timeline) {
        ImGui::Text("Execute Pass: %s\n", pass->name.c_str());
    }
}

void RenderGraph::Print() {
    if (!compiled) Compile();

    for (auto pass : timeline) {
        std::cout << "Execute Pass: " << pass->name << std::endl;
    }
    std::cout << "---------------" << std::endl;
}
