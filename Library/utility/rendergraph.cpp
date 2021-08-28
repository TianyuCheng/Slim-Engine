#include <numeric>
#include <imgui.h>
#include "core/debug.h"
#include "core/vkutils.h"
#include "utility/rendergraph.h"

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
    VkImageUsageFlags usage = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
                            | VK_IMAGE_USAGE_SAMPLED_BIT;

    if (IsDepthStencil(format)) {
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    } else {
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    if (!image.get()) {
        image = renderFrame->RequestGPUImage(format, extent, mipLevels, 1, samples, usage);
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
}

void RenderGraph::Subpass::SetColor(RenderGraph::Resource *resource) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::ColorAttachment });
    resource->writers.push_back(parent);
    usedAsColorAttachment.push_back(attachmentId);
}

void RenderGraph::Subpass::SetColor(RenderGraph::Resource *resource, const ClearValue &clear) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::ColorAttachment, clear });
    resource->writers.push_back(parent);
    usedAsColorAttachment.push_back(attachmentId);
}

void RenderGraph::Subpass::SetDepth(RenderGraph::Resource *resource) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::DepthAttachment });
    resource->writers.push_back(parent);
    usedAsDepthAttachment.push_back(attachmentId);
}

void RenderGraph::Subpass::SetDepth(RenderGraph::Resource *resource, const ClearValue &clear) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::DepthAttachment, clear });
    resource->writers.push_back(parent);
    usedAsDepthAttachment.push_back(attachmentId);
}

void RenderGraph::Subpass::SetStencil(RenderGraph::Resource *resource) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::StencilAttachment });
    resource->writers.push_back(parent);
    usedAsStencilAttachment.push_back(attachmentId);
}

void RenderGraph::Subpass::SetStencil(RenderGraph::Resource *resource, const ClearValue &clear) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::StencilAttachment, clear });
    resource->writers.push_back(parent);
    usedAsStencilAttachment.push_back(attachmentId);
}

void RenderGraph::Subpass::SetDepthStencil(RenderGraph::Resource *resource) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::DepthStencilAttachment });
    resource->writers.push_back(parent);
    usedAsDepthStencilAttachment.push_back(attachmentId);
}

void RenderGraph::Subpass::SetDepthStencil(RenderGraph::Resource *resource, const ClearValue &clear) {
    uint32_t attachmentId = parent->AddAttachment(ResourceMetadata { resource, ResourceType::DepthStencilAttachment, clear });
    resource->writers.push_back(parent);
    usedAsDepthStencilAttachment.push_back(attachmentId);
}

void RenderGraph::Subpass::SetTexture(RenderGraph::Resource *resource) {
    uint32_t textureId = parent->AddTexture(resource);
    resource->readers.push_back(parent);
    usedAsTexture.push_back(textureId);
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

    if (compute) {
        ExecuteCompute(commandBuffer);
    } else {
        ExecuteGraphics(commandBuffer);
    }

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

    // texture layout transition
    for (auto &textureId : defaultSubpass->usedAsTexture) {
        textures[textureId]->image->layouts[0][0] = textures[textureId]->layout;
        commandBuffer->PrepareForShaderRead(textures[textureId]->image);
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
        }
        if (attachment.clearValue.has_value()) {
            clearValues.push_back(attachment.clearValue.value());
        } else {
            clearValues.push_back(ClearValue(1.0, 1.0, 1.0, 1.0));
        }
        attachmentIds.push_back(attachmentId);
    }

    // texture layout transition
    for (auto &texture : textures) {
        // transit dst image layout
        PrepareLayoutTransition(*commandBuffer, texture->image,
            texture->layout,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
            0, texture->image->Layers(),
            0, texture->image->MipLevels());
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
            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            subpassDesc.AddDepthAttachment(attachmentId, initialLayout, finalLayout);
            attachments[attachment].resource->layout = finalLayout;
        }
        for (uint32_t attachment : subpass->usedAsStencilAttachment) {
            uint32_t attachmentId = attachmentIds[attachment];
            VkImageLayout initialLayout = attachments[attachment].resource->layout;
            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
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

    // adding this pass in a post-order
    timeline.push_back(pass);
}

void RenderGraph::UpdateCommandBufferDependencies(CommandBuffer* commandBuffer, Resource* resource) const {
    for (const auto& writerPass : resource->writers) {
        if (!writerPass->commandBuffer) continue;

        for (const auto& meta : writerPass->attachments) {
            if (meta.resource == resource) {
                switch (meta.type) {
                    case ResourceType::ColorAttachment:
                    case ResourceType::ColorResolveAttachment:
                        commandBuffer->Wait(writerPass->signalSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
                        break;
                    case ResourceType::DepthAttachment:
                    case ResourceType::StencilAttachment:
                    case ResourceType::DepthStencilAttachment:
                        commandBuffer->Wait(writerPass->signalSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
                        break;
                }
                return;
            }
        }

        // for all other types
        commandBuffer->Wait(writerPass->signalSemaphore, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        return;
    }
}

bool RenderGraph::HasComputePassDependency(Pass* pass) const {
    // input
    for (const auto& texture : pass->textures) {
        for (const auto& writerPass : texture->writers) {
            if (writerPass->compute) {
                return true;
            }
        }
    }
    return false;
}

void RenderGraph::Execute() {
    if (!compiled) Compile();

    // NOTE: Technically we will need a compute buffer for each compute pass.
    // For dependency synchronization, any draw pass that needs input from compute pass
    // would require separate command buffer;

    // remember last graphics command buffer
    CommandBuffer* lastGraphicsCommandBuffer = nullptr;
    Semaphore* lastGraphicsSemaphore = nullptr;

    for (Pass *pass : timeline) {
        // don't execute a pass if it is executed
        // theoretically it should have been taken care of by Compile step
        if (pass->visited) continue;
        if (!pass->retained) continue;

        CommandBuffer* commandBuffer = nullptr;

        // create new command buffer for each pass
        if (pass->compute) {
            pass->commandBuffer = renderFrame->RequestCommandBuffer(VK_QUEUE_COMPUTE_BIT);
            pass->signalSemaphore = renderFrame->RequestSemaphore();
            pass->commandBuffer->Signal(pass->signalSemaphore); // all compute passes need to signal, otherwise it should be culled
            pass->commandBuffer->Begin();
            commandBuffer = pass->commandBuffer;
            // execute the pass
            pass->Execute(pass->commandBuffer);
        } else {
            if (HasComputePassDependency(pass)) {
                // request new command buffer
                pass->commandBuffer = renderFrame->RequestCommandBuffer(VK_QUEUE_GRAPHICS_BIT);
                pass->signalSemaphore = renderFrame->RequestSemaphore();
                // update dependencies between command buffer
                if (lastGraphicsCommandBuffer && lastGraphicsSemaphore) {
                    lastGraphicsCommandBuffer->Signal(lastGraphicsSemaphore);                               // signal from previous graphics command buffer
                    pass->commandBuffer->Wait(lastGraphicsSemaphore, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);   // wait from current graphics command buffer
                }
                // update current command buffer handles
                lastGraphicsCommandBuffer = pass->commandBuffer;
                lastGraphicsSemaphore = pass->signalSemaphore;
                lastGraphicsCommandBuffer->Begin();
            }
            if (!lastGraphicsCommandBuffer) {
                lastGraphicsCommandBuffer = renderFrame->RequestCommandBuffer(VK_QUEUE_GRAPHICS_BIT);
                lastGraphicsSemaphore = renderFrame->RequestSemaphore();
                lastGraphicsCommandBuffer->Begin();
            }
            // execute the pass
            pass->Execute(lastGraphicsCommandBuffer);
        }
        pass->visited = true;

        // resolving command buffer dependencies
        for (const auto& resource : pass->textures) {
            UpdateCommandBufferDependencies(pass->commandBuffer, resource);
        }
    }

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
        lastGraphicsCommandBuffer->PrepareForPresentSrc(backBuffer);
        lastGraphicsCommandBuffer->End();
    }

    // stop command buffer recording for all
    for (Pass *pass : timeline) {
        if (pass->commandBuffer && pass->commandBuffer != lastGraphicsCommandBuffer) {
            pass->commandBuffer->End();
        }
    }

    // submit all command buffers
    std::vector<VkSubmitInfo> computeSubmits = {};
    std::vector<VkSubmitInfo> graphicsSubmits = {};
    for (Pass *pass : timeline) {
        if (pass->commandBuffer && pass->commandBuffer.get() != lastGraphicsCommandBuffer) {
            if (pass->compute) {
                computeSubmits.push_back(pass->commandBuffer->GetSubmitInfo());
            } else {
                graphicsSubmits.push_back(pass->commandBuffer->GetSubmitInfo());
            }
        }
    }

    Device* device = renderFrame->GetDevice();
    if (computeSubmits.size()) {
        ErrorCheck(DeviceDispatch(vkQueueSubmit(device->GetComputeQueue(),
                                 computeSubmits.size(), computeSubmits.data(),
                                 *renderFrame->GetComputeFinishFence())),
                    "submit compute commands");
    }
    if (graphicsSubmits.size()) {
        ErrorCheck(DeviceDispatch(vkQueueSubmit(device->GetGraphicsQueue(),
                                 graphicsSubmits.size(), graphicsSubmits.data(),
                                 *renderFrame->GetGraphicsFinishFence())),
                    "submit graphics commands");
    }
    if (lastGraphicsCommandBuffer) {
        renderFrame->Present(lastGraphicsCommandBuffer);
    }
}

void RenderGraph::Visualize() {
    if (!compiled) Compile();

    for (auto pass : timeline) {
        ImGui::Text("Execute Pass: %s\n", pass->name.c_str());
    }
}
