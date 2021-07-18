#include "core/debug.h"
#include "core/vkutils.h"
#include "core/renderpass.h"

using namespace slim;

ClearValue::ClearValue(float r, float g, float b, float a) {
    handle.color.float32[0] = r;
    handle.color.float32[1] = g;
    handle.color.float32[2] = b;
    handle.color.float32[3] = a;
}

ClearValue::ClearValue(float depth, uint32_t stencil) {
    handle.depthStencil.depth = depth;
    handle.depthStencil.stencil = stencil;
}

RenderPassDesc& RenderPassDesc::AddColorAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                                   VkAttachmentLoadOp load, VkAttachmentStoreOp store,
                                                   VkImageLayout initialLayout, VkImageLayout finalLayout) {
    uint32_t index = attachments.size();

    // description
    attachments.push_back(VkAttachmentDescription {});
    VkAttachmentDescription &desc = attachments.back();
    desc.format = format;
    desc.samples = samples;
    desc.loadOp = load;
    desc.storeOp = store;
    desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.initialLayout = initialLayout;
    desc.finalLayout = finalLayout;
    desc.flags = 0;

    // reference
    colorAttachments.push_back(VkAttachmentReference {
        index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    });

    return *this;
}

RenderPassDesc& RenderPassDesc::AddDepthAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                                   VkAttachmentLoadOp load, VkAttachmentStoreOp store,
                                                   VkImageLayout initialLayout, VkImageLayout finalLayout) {
    uint32_t index = attachments.size();

    attachments.push_back(VkAttachmentDescription {});
    VkAttachmentDescription &desc = attachments.back();
    desc.format = format;
    desc.samples = samples;
    desc.loadOp = load;
    desc.storeOp = store;
    desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.initialLayout = initialLayout;
    desc.finalLayout = finalLayout;
    desc.flags = 0;

    // reference
    depthStencilAttachments.push_back(VkAttachmentReference {
        index, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
    });

    return *this;
}

RenderPassDesc& RenderPassDesc::AddStencilAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                                     VkAttachmentLoadOp load, VkAttachmentStoreOp store,
                                                     VkImageLayout initialLayout, VkImageLayout finalLayout) {
    uint32_t index = attachments.size();

    attachments.push_back(VkAttachmentDescription {});
    VkAttachmentDescription &desc = attachments.back();
    desc.format = format;
    desc.samples = samples;
    desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.stencilLoadOp = load;
    desc.stencilStoreOp = store;
    desc.initialLayout = initialLayout;
    desc.finalLayout = finalLayout;
    desc.flags = 0;

    // reference
    depthStencilAttachments.push_back(VkAttachmentReference {
        index, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
    });

    return *this;
}

RenderPassDesc& RenderPassDesc::AddDepthStencilAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                                          VkAttachmentLoadOp load, VkAttachmentStoreOp store,
                                                          VkImageLayout initialLayout, VkImageLayout finalLayout) {
    uint32_t index = attachments.size();

    attachments.push_back(VkAttachmentDescription {});
    VkAttachmentDescription &desc = attachments.back();
    desc.format = format;
    desc.samples = samples;
    desc.loadOp = load;
    desc.storeOp = store;
    desc.stencilLoadOp = load;
    desc.stencilStoreOp = store;
    desc.initialLayout = initialLayout;
    desc.finalLayout = finalLayout;
    desc.flags = 0;

    // reference
    depthStencilAttachments.push_back(VkAttachmentReference {
        index, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    });

    return *this;
}

RenderPassDesc& RenderPassDesc::AddResolveAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                                     VkAttachmentLoadOp load, VkAttachmentStoreOp store,
                                                     VkImageLayout initialLayout, VkImageLayout finalLayout) {
    uint32_t index = attachments.size();

    // description
    attachments.push_back(VkAttachmentDescription {});
    VkAttachmentDescription &desc = attachments.back();
    desc.format = format;
    desc.samples = samples;
    desc.loadOp = load;
    desc.storeOp = store;
    desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.initialLayout = initialLayout;
    desc.finalLayout = finalLayout;
    desc.flags = 0;

    // reference
    resolveAttachments.push_back(VkAttachmentReference {
        index,
        IsDepthStencil(format) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                               : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    });

    return *this;
}

RenderPass::RenderPass(Context *context, const RenderPassDesc &desc)
    : context(context) {

    #ifndef NDEBUG
    assert(desc.name.length() > 0 && "RenderPassDesc must have a name!");
    #endif

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = desc.colorAttachments.size();
    subpass.pColorAttachments = desc.colorAttachments.data();
    if (desc.depthStencilAttachments.size())
        subpass.pDepthStencilAttachment = desc.depthStencilAttachments.data();
    if (desc.resolveAttachments.size())
        subpass.pResolveAttachments = desc.resolveAttachments.data();

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = desc.attachments.size();
    renderPassInfo.pAttachments = desc.attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    ErrorCheck(vkCreateRenderPass(context->GetDevice(), &renderPassInfo, nullptr, &handle), "create render pass");
}

RenderPass::~RenderPass() {
    if (handle) {
        vkDestroyRenderPass(context->GetDevice(), handle, nullptr);
        handle = VK_NULL_HANDLE;
    }
}
