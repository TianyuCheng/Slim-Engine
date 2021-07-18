#include <numeric>
#include <imgui.h>
#include "core/debug.h"
#include "core/vkutils.h"
#include "utility/rendergraph.h"

using namespace slim;

bool IsDepthStencil(VkFormat format) {
    return format == VK_FORMAT_D16_UNORM
         | format == VK_FORMAT_D16_UNORM_S8_UINT
         | format == VK_FORMAT_D24_UNORM_S8_UINT
         | format == VK_FORMAT_D32_SFLOAT
         | format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}

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

void RenderGraph::Pass::Execute(std::function<void(const RenderGraph &renderGraph)> cb) {
    callback = cb;
}

void RenderGraph::Pass::Execute() {
    RenderFrame* renderFrame = graph->GetRenderFrame();

    // allocate resource if necessary
    // no need to allocate texture, because it should already have been allocated
    for (auto &attachment : usedAsColorAttachment)        attachment.resource->Allocate(renderFrame);
    for (auto &attachment : usedAsDepthAttachment)        attachment.resource->Allocate(renderFrame);
    for (auto &attachment : usedAsStencilAttachment)      attachment.resource->Allocate(renderFrame);
    for (auto &attachment : usedAsDepthStencilAttachment) attachment.resource->Allocate(renderFrame);

    if (compute) {
        ExecuteCompute();
    } else {
        ExecuteGraphics();
    }

    // update reference counts
    for (auto &attachment : usedAsTexture)                attachment.resource->rdCount--;
    for (auto &attachment : usedAsColorAttachment)        attachment.resource->wrCount--;
    for (auto &attachment : usedAsDepthAttachment)        attachment.resource->wrCount--;
    for (auto &attachment : usedAsStencilAttachment)      attachment.resource->wrCount--;
    for (auto &attachment : usedAsDepthStencilAttachment) attachment.resource->wrCount--;

    // clean up resources not used anymore
    for (auto &attachment : usedAsTexture)                if (attachment.resource->rdCount + attachment.resource->wrCount == 0) attachment.resource->Deallocate();
    for (auto &attachment : usedAsColorAttachment)        if (attachment.resource->rdCount + attachment.resource->wrCount == 0) attachment.resource->Deallocate();
    for (auto &attachment : usedAsDepthAttachment)        if (attachment.resource->rdCount + attachment.resource->wrCount == 0) attachment.resource->Deallocate();
    for (auto &attachment : usedAsStencilAttachment)      if (attachment.resource->rdCount + attachment.resource->wrCount == 0) attachment.resource->Deallocate();
    for (auto &attachment : usedAsDepthStencilAttachment) if (attachment.resource->rdCount + attachment.resource->wrCount == 0) attachment.resource->Deallocate();
}

void RenderGraph::Pass::ExecuteCompute() {
    CommandBuffer* commandBuffer = graph->GetComputeCommandBuffer();

    // texture layout transition
    for (auto &texture : usedAsTexture) {
        commandBuffer->PrepareForShaderRead(texture.resource->image);
    }

    // execute compute callback
    callback(*graph);
}

void RenderGraph::Pass::ExecuteGraphics() {
    // prepare a render pass
    RenderPassDesc renderPassDesc;
    FramebufferDesc framebufferDesc;
    std::vector<VkClearValue> clearValues;

    CommandBuffer* commandBuffer = graph->GetGraphicsCommandBuffer();

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
    beginInfo.renderArea.extent = graph->GetRenderFrame()->GetExtent();
    beginInfo.clearValueCount = clearValues.size();
    beginInfo.pClearValues = clearValues.data();
    beginInfo.pNext = nullptr;
    vkCmdBeginRenderPass(*commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // execute draw callback
    callback(*graph);

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

CommandBuffer* RenderGraph::GetGraphicsCommandBuffer() const {
    if (!graphicsCommandBuffer.get()) {
        graphicsCommandBuffer = renderFrame->RequestCommandBuffer(VK_QUEUE_GRAPHICS_BIT);
        graphicsCommandBuffer->Begin();
    }
    return graphicsCommandBuffer.get();
}

CommandBuffer* RenderGraph::GetComputeCommandBuffer() const {
    if (!computeCommandBuffer.get()) {
        computeCommandBuffer = renderFrame->RequestCommandBuffer(VK_QUEUE_COMPUTE_BIT);
        computeCommandBuffer->Begin();
    }
    return computeCommandBuffer.get();
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

void RenderGraph::Execute() {
    if (!compiled) Compile();

    for (Pass *pass : timeline) {
        // don't execute a pass if it is executed
        // theoretically it should have been taken care of by Compile step
        if (pass->visited) continue;
        if (!pass->retained) continue;

        // execute and mark the pass
        pass->Execute();
        pass->visited = true;
    }

    if (computeCommandBuffer.get()) {
        computeCommandBuffer->End();
    }

    if (graphicsCommandBuffer.get()) {
        // trans
        GPUImage2D* backbuffer = renderFrame->GetBackBuffer();
        graphicsCommandBuffer->PrepareForPresentSrc(backbuffer);
        graphicsCommandBuffer->End();
        renderFrame->Present(GetGraphicsCommandBuffer());
    }
}

void RenderGraph::Visualize() {
    if (!compiled) Compile();

    for (auto pass : timeline) {
        ImGui::Text("Execute Pass: %s\n", pass->name.c_str());
    }
}
