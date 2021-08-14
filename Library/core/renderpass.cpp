#include "core/debug.h"
#include "core/hasher.h"
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

SubpassDesc::SubpassDesc(RenderPassDesc* parent, uint32_t subpassIndex)
    : parent(parent), subpassIndex(subpassIndex) {
}

SubpassDesc& SubpassDesc::AddColorAttachment(uint32_t attachment, VkImageLayout inLayout, VkImageLayout outLayout) {

    // reference
    colorAttachments.push_back(VkAttachmentReference {
        attachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    });

    // record layout transition in mappings
    layoutTransitionMap[attachment] = std::make_tuple(inLayout, outLayout);

    // record attachment-subpass relationships
    parent->subpassesWrites[subpassIndex].insert(attachment);

    return *this;
}

SubpassDesc& SubpassDesc::AddDepthAttachment(uint32_t attachment, VkImageLayout inLayout, VkImageLayout outLayout) {

    // reference
    depthStencilAttachments.push_back(VkAttachmentReference {
        attachment, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
    });

    // record layout transition in mappings
    layoutTransitionMap[attachment] = std::make_tuple(inLayout, outLayout);

    // record attachment-subpass relationships
    parent->subpassesWrites[subpassIndex].insert(attachment);

    return *this;
}

SubpassDesc& SubpassDesc::AddStencilAttachment(uint32_t attachment, VkImageLayout inLayout, VkImageLayout outLayout) {

    // reference
    depthStencilAttachments.push_back(VkAttachmentReference {
        attachment, VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL
    });

    // record layout transition in mappings
    layoutTransitionMap[attachment] = std::make_tuple(inLayout, outLayout);

    // record attachment-subpass relationships
    parent->subpassesWrites[subpassIndex].insert(attachment);

    return *this;
}

SubpassDesc& SubpassDesc::AddDepthStencilAttachment(uint32_t attachment, VkImageLayout inLayout, VkImageLayout outLayout) {

    // reference
    depthStencilAttachments.push_back(VkAttachmentReference {
        attachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    });

    // record layout transition in mappings
    layoutTransitionMap[attachment] = std::make_tuple(inLayout, outLayout);

    // record attachment-subpass relationships
    parent->subpassesWrites[subpassIndex].insert(attachment);

    return *this;
}

SubpassDesc& SubpassDesc::AddResolveAttachment(uint32_t attachment, VkImageLayout inLayout, VkImageLayout outLayout) {
    VkFormat format = parent->attachments[attachment].format;

    // reference
    resolveAttachments.push_back(VkAttachmentReference {
        attachment,
        IsDepthStencil(format) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                               : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    });

    // record layout transition in mappings
    layoutTransitionMap[attachment] = std::make_tuple(inLayout, outLayout);

    // record attachment-subpass relationships
    parent->subpassesWrites[subpassIndex].insert(attachment);

    return *this;
}

SubpassDesc& SubpassDesc::AddPreserveAttachment(uint32_t attachment) {
    // reference
    preserveAttachments.push_back(attachment);

    // record attachment-subpass relationships
    parent->subpassesReads[subpassIndex].insert(attachment);
    parent->subpassesWrites[subpassIndex].insert(attachment);

    return *this;
}

SubpassDesc& RenderPassDesc::AddSubpass() {
    subpasses.push_back(SubpassDesc(this, subpasses.size()));

    // add additional info to record attachment relationships with subpasses
    subpassesReads.push_back(std::unordered_set<uint32_t> {});
    subpassesWrites.push_back(std::unordered_set<uint32_t> {});

    return subpasses.back();
}

uint32_t RenderPassDesc::AddAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                       VkAttachmentLoadOp load, VkAttachmentStoreOp store,
                                       VkAttachmentLoadOp stencilLoad, VkAttachmentStoreOp stencilStore) {
    uint32_t index = attachments.size();

    // update attachment info
    attachments.push_back(VkAttachmentDescription { });
    VkAttachmentDescription& description = attachments.back();
    description.format = format;
    description.samples = samples;
    description.loadOp = load;
    description.storeOp = store;
    description.stencilLoadOp = stencilLoad;
    description.stencilStoreOp = stencilStore;
    description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    description.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    description.flags = 0;

    return index;
}

uint32_t RenderPassDesc::AddColorAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                            VkAttachmentLoadOp load, VkAttachmentStoreOp store) {
    return AddAttachment(format, samples, load, store, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE);
}

uint32_t RenderPassDesc::AddColorResolveAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                                   VkAttachmentLoadOp load, VkAttachmentStoreOp store) {
    return AddAttachment(format, samples, load, store, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE);
}

uint32_t RenderPassDesc::AddDepthAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                            VkAttachmentLoadOp load, VkAttachmentStoreOp store) {
    return AddAttachment(format, samples, load, store, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE);
}

uint32_t RenderPassDesc::AddStencilAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                              VkAttachmentLoadOp load, VkAttachmentStoreOp store) {
    return AddAttachment(format, samples, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, load, store);
}

uint32_t RenderPassDesc::AddDepthStencilAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                                   VkAttachmentLoadOp load, VkAttachmentStoreOp store) {
    return AddAttachment(format, samples, load, store, load, store);
}

RenderPass::RenderPass(Device *device, const RenderPassDesc &desc)
    : device(device) {

    #ifndef NDEBUG
    // RenderPass object must have a name, because RenderFrame uses its name for hashing
    assert(desc.name.length() > 0 && "RenderPassDesc must have a name!");
    #endif

    // subpasses
    std::vector<VkSubpassDescription> subpasses = {};
    for (const SubpassDesc& subpassDesc : desc.subpasses) {
        subpasses.push_back(VkSubpassDescription {});
        VkSubpassDescription& subpass = subpasses.back();
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = subpassDesc.colorAttachments.size();
        subpass.pColorAttachments = subpassDesc.colorAttachments.data();
        subpass.inputAttachmentCount = subpassDesc.inputAttachments.size();
        subpass.pInputAttachments = subpassDesc.inputAttachments.data();

        // depth stencil
        if (!subpassDesc.depthStencilAttachments.empty()) {
            #ifndef NDEBUG
            if (subpassDesc.depthStencilAttachments.size() > 1) {
                throw std::runtime_error("[RenderPass] encounter more than 1 depth stencil buffer");
            }
            #endif
            subpass.pDepthStencilAttachment = subpassDesc.depthStencilAttachments.data();
        } else {
            subpass.pDepthStencilAttachment = nullptr;
        }

        // color resolve
        if (!subpassDesc.resolveAttachments.empty()) {
            #ifndef NDEBUG
            if (subpassDesc.resolveAttachments.size() != subpassDesc.colorAttachments.size()) {
                throw std::runtime_error("[RenderPass] encounter mismatching number of resolve attachments from color attachment");
            }
            #endif
            subpass.pResolveAttachments = subpassDesc.resolveAttachments.data();
        } else {
            subpass.pResolveAttachments = nullptr;
        }
    }

    // dependencies
    std::vector<VkSubpassDependency> dependencies = {};
    if (desc.subpasses.size() == 1) {
        dependencies.push_back(VkSubpassDependency { });

        VkSubpassDependency& dependency = dependencies.back();
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // fill initial layout and final layout
        for (const auto& subpass : desc.subpasses) {
            for (const auto& kv : subpass.layoutTransitionMap) {
                auto [initialLayout, finalLayout] = kv.second;
                desc.attachments[kv.first].initialLayout = initialLayout;
                desc.attachments[kv.first].finalLayout = finalLayout;
            }
        }

    } else {
        assert(!!!"Implement this later");
        // std::unordered_set<std::tuple<uint32_t, uint32_t>> deps;
        //
        // for (uint32_t i = 0; i < desc.subpasses.size(); i++) {
        //     const auto& subpass1reads = desc.subpassesReads[i];
        //     const auto& subpass1writes = desc.subpassesWrites[i];
        //     for (uint32_t j = 0; j < desc.subpasses.size(); j++) {
        //         const auto& subpass2reads = desc.subpassesReads[j];
        //         const auto& subpass2writes = desc.subpassesWrites[j];
        //     }
        // }
        // TODO: figure out attachments initial layout and final layout
    }

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = desc.attachments.size();
    renderPassInfo.pAttachments = desc.attachments.data();
    renderPassInfo.subpassCount = subpasses.size();
    renderPassInfo.pSubpasses = subpasses.data();
    renderPassInfo.dependencyCount = dependencies.size();
    renderPassInfo.pDependencies = dependencies.data();

    ErrorCheck(vkCreateRenderPass(*device, &renderPassInfo, nullptr, &handle), "create render pass");
}

RenderPass::~RenderPass() {
    if (handle) {
        vkDestroyRenderPass(*device, handle, nullptr);
        handle = VK_NULL_HANDLE;
    }
}
