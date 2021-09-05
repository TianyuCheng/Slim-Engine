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

Framebuffer::Framebuffer(Device *device, FramebufferDesc &desc)
    : device(device) {

    desc.handle.attachmentCount = desc.attachments.size();
    desc.handle.pAttachments = desc.attachments.data();

    ErrorCheck(DeviceDispatch(vkCreateFramebuffer(*device, &desc.handle, nullptr, &handle)), "create framebuffer");
}

Framebuffer::~Framebuffer() {
    if (handle) {
        DeviceDispatch(vkDestroyFramebuffer(*device, handle, nullptr));
    }
}

void Framebuffer::SetName(const std::string& name) const {
    if (device->IsDebuggerEnabled()) {
        VkDebugMarkerObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT;
        nameInfo.object = (uint64_t) handle;
        nameInfo.pObjectName = name.c_str();
        ErrorCheck(DeviceDispatch(vkDebugMarkerSetObjectNameEXT(*device, &nameInfo)), "set framebuffer name");
    }
}
