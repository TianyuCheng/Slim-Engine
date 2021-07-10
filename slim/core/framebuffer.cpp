#include <numeric>
#include "core/debug.h"
#include "core/vkutils.h"
#include "core/framebuffer.h"

using namespace slim;

FramebufferDesc::FramebufferDesc() {
    handle = {};
    handle.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    handle.width = 0;
    handle.height = 0;
    handle.layers = 1;
    handle.pNext = nullptr;
    handle.renderPass = VK_NULL_HANDLE;
    handle.pNext = nullptr;
}

FramebufferDesc& FramebufferDesc::SetExtent(uint32_t width, uint32_t height) {
    handle.width = width;
    handle.height = height;
    return *this;
}

FramebufferDesc& FramebufferDesc::SetLayers(uint32_t layers) {
    handle.layers = layers;
    return *this;
}

FramebufferDesc& FramebufferDesc::SetRenderPass(RenderPass *renderPass) {
    handle.renderPass = *renderPass;
    return *this;
}

FramebufferDesc& FramebufferDesc::AddAttachment(VkImageView view) {
    attachments.push_back(view);
    return *this;
}

Framebuffer::Framebuffer(Context *context, FramebufferDesc &desc)
    : context(context) {

    desc.handle.attachmentCount = desc.attachments.size();
    desc.handle.pAttachments = desc.attachments.data();

    ErrorCheck(vkCreateFramebuffer(context->GetDevice(), &desc.handle, nullptr, &handle), "create framebuffer");
}

Framebuffer::~Framebuffer() {
    if (handle) {
        vkDestroyFramebuffer(context->GetDevice(), handle, nullptr);
    }
}
