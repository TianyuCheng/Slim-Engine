#include <numeric>
#include <imgui.h>
#include "core/debug.h"
#include "core/vkutils.h"
#include "utility/rendergraph.h"

using namespace slim;

RenderGraph::Resource::Resource(GPUImage2D* image)
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
        image = renderFrame->RequestGPUImage2D(format, extent, mipLevels, samples, usage);
    }
}

void RenderGraph::Resource::Deallocate() {
    image.Reset();
}

RenderGraph::Pass::Pass(const std::string &name, RenderGraph *graph, bool compute) : name(name), graph(graph), compute(compute) {
}

void RenderGraph::Pass::SetColorResolve(RenderGraph::Resource *resource) {
    attachments.push_back(resource);
    resource->writers.push_back(this);
    usedAsColorResolveAttachment.push_back({ resource });
}

void RenderGraph::Pass::SetColor(RenderGraph::Resource *resource) {
    attachments.push_back(resource);
    resource->writers.push_back(this);
    usedAsColorAttachment.push_back({ resource });
}

void RenderGraph::Pass::SetColor(RenderGraph::Resource *resource, const ClearValue &clear) {
    attachments.push_back(resource);
    resource->writers.push_back(this);
    usedAsColorAttachment.push_back({ resource, clear });
}

void RenderGraph::Pass::SetDepth(RenderGraph::Resource *resource) {
    attachments.push_back(resource);
    resource->writers.push_back(this);
    usedAsDepthAttachment.push_back({ resource });
}

void RenderGraph::Pass::SetDepth(RenderGraph::Resource *resource, const ClearValue &clear) {
    attachments.push_back(resource);
    resource->writers.push_back(this);
    usedAsDepthAttachment.push_back({ resource, clear });
}

void RenderGraph::Pass::SetStencil(RenderGraph::Resource *resource) {
    attachments.push_back(resource);
    resource->writers.push_back(this);
    usedAsStencilAttachment.push_back({ resource });
}

void RenderGraph::Pass::SetStencil(RenderGraph::Resource *resource, const ClearValue &clear) {
    attachments.push_back(resource);
    resource->writers.push_back(this);
    usedAsStencilAttachment.push_back({ resource, clear });
}

void RenderGraph::Pass::SetDepthStencil(RenderGraph::Resource *resource) {
    attachments.push_back(resource);
    resource->writers.push_back(this);
    usedAsDepthStencilAttachment.push_back({ resource });
}

void RenderGraph::Pass::SetDepthStencil(RenderGraph::Resource *resource, const ClearValue &clear) {
    attachments.push_back(resource);
    resource->writers.push_back(this);
    usedAsDepthStencilAttachment.push_back({ resource, clear });
}

void RenderGraph::Pass::SetTexture(RenderGraph::Resource *resource) {
    resource->readers.push_back(this);
    usedAsTexture.push_back({ resource });
}

void RenderGraph::Pass::Execute(std::function<void(const RenderInfo &renderInfo)> cb) {
    callback = cb;
}

void RenderGraph::Pass::Execute(CommandBuffer* commandBuffer) {
    RenderFrame* renderFrame = graph->GetRenderFrame();

    // allocate resource if necessary
    // no need to allocate texture, because it should already have been allocated
    for (auto &attachment : usedAsColorAttachment)        attachment.resource->Allocate(renderFrame);
    for (auto &attachment : usedAsDepthAttachment)        attachment.resource->Allocate(renderFrame);
    for (auto &attachment : usedAsStencilAttachment)      attachment.resource->Allocate(renderFrame);
    for (auto &attachment : usedAsDepthStencilAttachment) attachment.resource->Allocate(renderFrame);
    for (auto &attachment : usedAsColorResolveAttachment) attachment.resource->Allocate(renderFrame);

    if (compute) {
        ExecuteCompute(commandBuffer);
    } else {
        ExecuteGraphics(commandBuffer);
    }

    // update reference counts
    for (auto &attachment : usedAsTexture)                attachment.resource->rdCount--;
    for (auto &attachment : usedAsColorAttachment)        attachment.resource->wrCount--;
    for (auto &attachment : usedAsDepthAttachment)        attachment.resource->wrCount--;
    for (auto &attachment : usedAsStencilAttachment)      attachment.resource->wrCount--;
    for (auto &attachment : usedAsDepthStencilAttachment) attachment.resource->wrCount--;
    for (auto &attachment : usedAsColorResolveAttachment) attachment.resource->wrCount--;

    // clean up resources not used anymore
    for (auto &attachment : usedAsTexture)                if (attachment.resource->rdCount + attachment.resource->wrCount == 0) attachment.resource->Deallocate();
    for (auto &attachment : usedAsColorAttachment)        if (attachment.resource->rdCount + attachment.resource->wrCount == 0) attachment.resource->Deallocate();
    for (auto &attachment : usedAsDepthAttachment)        if (attachment.resource->rdCount + attachment.resource->wrCount == 0) attachment.resource->Deallocate();
    for (auto &attachment : usedAsStencilAttachment)      if (attachment.resource->rdCount + attachment.resource->wrCount == 0) attachment.resource->Deallocate();
    for (auto &attachment : usedAsDepthStencilAttachment) if (attachment.resource->rdCount + attachment.resource->wrCount == 0) attachment.resource->Deallocate();
    for (auto &attachment : usedAsColorResolveAttachment) if (attachment.resource->rdCount + attachment.resource->wrCount == 0) attachment.resource->Deallocate();
}

void RenderGraph::Pass::ExecuteCompute(CommandBuffer* commandBuffer) {
    // texture layout transition
    for (auto &texture : usedAsTexture) {
        commandBuffer->PrepareForShaderRead(texture.resource->image);
    }

    // TODO: add other types of resources which needs layout transition

    // execute compute callback
    RenderInfo info;
    info.renderGraph = graph;
    info.renderFrame = graph->GetRenderFrame();
    info.renderPass = nullptr;
    info.commandBuffer = commandBuffer;
    callback(info);
}

void RenderGraph::Pass::ExecuteGraphics(CommandBuffer* commandBuffer) {
    // prepare a render pass
    RenderPassDesc renderPassDesc;
    FramebufferDesc framebufferDesc;
    std::vector<VkClearValue> clearValues;

    auto inferLoadOp = [](const ResourceMetadata &attachment) -> VkAttachmentLoadOp {
        if (attachment.clearValue.has_value())
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        if (attachment.resource->layout == VK_IMAGE_LAYOUT_UNDEFINED)
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    };

    // color attachments
    for (auto &attachment : usedAsColorAttachment) {
        renderPassDesc.AddColorAttachment(attachment.resource->format,
                                          attachment.resource->samples,
                                          inferLoadOp(attachment),
                                          VK_ATTACHMENT_STORE_OP_STORE,
                                          attachment.resource->layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        framebufferDesc.AddAttachment(attachment.resource->image->AsColorBuffer());
        attachment.resource->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        if (attachment.clearValue.has_value()) clearValues.push_back(attachment.clearValue.value());
    }

    // depth attachment
    for (auto &attachment : usedAsDepthAttachment) {
        renderPassDesc.AddDepthAttachment(attachment.resource->format,
                                          attachment.resource->samples,
                                          inferLoadOp(attachment),
                                          VK_ATTACHMENT_STORE_OP_STORE,
                                          attachment.resource->layout, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        framebufferDesc.AddAttachment(attachment.resource->image->AsDepthBuffer());
        attachment.resource->layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        if (attachment.clearValue.has_value()) clearValues.push_back(attachment.clearValue.value());
    }

    // stencil attachment
    for (auto &attachment : usedAsStencilAttachment) {
        renderPassDesc.AddStencilAttachment(attachment.resource->format,
                                            attachment.resource->samples,
                                            inferLoadOp(attachment),
                                            VK_ATTACHMENT_STORE_OP_STORE,
                                            attachment.resource->layout, VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL);
        framebufferDesc.AddAttachment(attachment.resource->image->AsStencilBuffer());
        attachment.resource->layout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
        if (attachment.clearValue.has_value()) clearValues.push_back(attachment.clearValue.value());
    }

    // depth stencil attachment
    for (auto &attachment : usedAsDepthStencilAttachment) {
        renderPassDesc.AddDepthStencilAttachment(attachment.resource->format,
                                            attachment.resource->samples,
                                            inferLoadOp(attachment),
                                            VK_ATTACHMENT_STORE_OP_STORE,
                                            attachment.resource->layout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        framebufferDesc.AddAttachment(attachment.resource->image->AsDepthStencilBuffer());
        attachment.resource->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        if (attachment.clearValue.has_value()) clearValues.push_back(attachment.clearValue.value());
    }

    // color resolve attachments
    for (auto &attachment : usedAsColorResolveAttachment) {
        renderPassDesc.AddResolveAttachment(attachment.resource->format,
                                            attachment.resource->samples,
                                            inferLoadOp(attachment),
                                            VK_ATTACHMENT_STORE_OP_STORE,
                                            attachment.resource->layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        framebufferDesc.AddAttachment(attachment.resource->image->AsColorBuffer());
        attachment.resource->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        if (attachment.clearValue.has_value()) clearValues.push_back(attachment.clearValue.value());
    }

    // texture layout transition
    for (auto &texture : usedAsTexture) {
        // transit dst image layout
        PrepareLayoutTransition(*commandBuffer, texture.resource->image,
            texture.resource->layout,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
            0, texture.resource->image->Layers(),
            0, texture.resource->image->MipLevels());
    }

    VkExtent2D extent;
    if (usedAsColorAttachment.size()) extent = usedAsColorAttachment[0].resource->extent;
    else if (usedAsDepthAttachment.size()) extent = usedAsDepthAttachment[0].resource->extent;
    else if (usedAsStencilAttachment.size()) extent = usedAsStencilAttachment[0].resource->extent;
    else if (usedAsDepthStencilAttachment.size()) extent = usedAsDepthStencilAttachment[0].resource->extent;
    else throw std::runtime_error("no attachment to detect extent!");
    framebufferDesc.SetExtent(extent.width, extent.height);

    // use a named render pass desc
    renderPassDesc.SetName(name);

    RenderFrame* renderFrame = graph->GetRenderFrame();
    RenderPass* renderPass = renderFrame->RequestRenderPass(renderPassDesc);
    Framebuffer* framebuffer = renderFrame->RequestFramebuffer(framebufferDesc.SetRenderPass(renderPass));

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
    vkCmdBeginRenderPass(*commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

    RenderInfo info;
    info.renderGraph = graph;
    info.renderFrame = renderFrame;
    info.renderPass = renderPass;
    info.commandBuffer = commandBuffer;

    // execute draw callback
    callback(info);

    // end render pass
    vkCmdEndRenderPass(*commandBuffer);
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

RenderGraph::Resource* RenderGraph::CreateResource(GPUImage2D* image) {
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
    // there must be at least 1 resource that is retained (for backbuffer)
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
    for (auto &meta : pass->usedAsTexture) CompileResource(meta.resource);

    // adding this pass in a post-order
    timeline.push_back(pass);
}

void RenderGraph::UpdateCommandBufferDependencies(CommandBuffer* commandBuffer, Resource* resource) const {
    for (const auto& writerPass : resource->writers) {
        if (!writerPass->commandBuffer) continue;
        // for color attachments
        for (const auto& res : writerPass->usedAsColorAttachment) {
            if (res.resource == resource) {
                commandBuffer->Wait(writerPass->signalSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
                return;
            }
        }
        for (const auto& res : writerPass->usedAsColorResolveAttachment) {
            if (res.resource == resource) {
                commandBuffer->Wait(writerPass->signalSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
                return;
            }
        }
        // for depth stencil attachments
        for (const auto& res : writerPass->usedAsDepthAttachment) {
            if (res.resource == resource) {
                commandBuffer->Wait(writerPass->signalSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
                return;
            }
        }
        for (const auto& res : writerPass->usedAsStencilAttachment) {
            if (res.resource == resource) {
                commandBuffer->Wait(writerPass->signalSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
                return;
            }
        }
        for (const auto& res : writerPass->usedAsDepthStencilAttachment) {
            if (res.resource == resource) {
                commandBuffer->Wait(writerPass->signalSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
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
    for (const auto& meta : pass->usedAsTexture)
        for (const auto& writerPass : meta.resource->writers)
            if (writerPass->compute)
                return true;
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
        for (const auto& meta : pass->usedAsTexture) {
            UpdateCommandBufferDependencies(pass->commandBuffer, meta.resource);
        }
    }

    // adding a layout transition
    if (lastGraphicsCommandBuffer) {
        // present src layout transition
        GPUImage2D* backbuffer = renderFrame->GetBackBuffer();
        lastGraphicsCommandBuffer->PrepareForPresentSrc(backbuffer);
    }

    // stop command buffer recording for all
    for (Pass *pass : timeline) {
        if (pass->commandBuffer && pass->commandBuffer != lastGraphicsCommandBuffer) {
            pass->commandBuffer->End();
        }
    }
    lastGraphicsCommandBuffer->End();

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
    if (computeSubmits.size()) {
        ErrorCheck(vkQueueSubmit(renderFrame->GetDevice()->GetComputeQueue(),
                                 computeSubmits.size(), computeSubmits.data(),
                                 *renderFrame->GetComputeFinishFence()),
                    "submit compute commands");
    }
    if (graphicsSubmits.size()) {
        ErrorCheck(vkQueueSubmit(renderFrame->GetDevice()->GetGraphicsQueue(),
                                 graphicsSubmits.size(), graphicsSubmits.data(),
                                 *renderFrame->GetGraphicsFinishFence()),
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
