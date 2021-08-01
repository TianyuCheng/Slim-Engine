#ifndef SLIM_CORE_RENDERPASS_H
#define SLIM_CORE_RENDERPASS_H

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "core/device.h"
#include "utility/interface.h"

namespace slim {

    class ClearValue final : public TriviallyConvertible<VkClearValue> {
    public:
        ClearValue(float r, float g, float b, float a = 1.0f);
        ClearValue(float depth, uint32_t stencil = 0U);
        ~ClearValue() = default;
    };

    inline bool IsDepthStencil(VkFormat format) {
        return format == VK_FORMAT_D16_UNORM
             | format == VK_FORMAT_D16_UNORM_S8_UINT
             | format == VK_FORMAT_D24_UNORM_S8_UINT
             | format == VK_FORMAT_D32_SFLOAT
             | format == VK_FORMAT_D32_SFLOAT_S8_UINT;
    }

    // RenderPassDesc should hold the data for all configurations needed for renderpass.
    // When renderpass is initialized, nothing should be changeable.
    class RenderPassDesc final {
        friend class RenderPass;
    public:
        RenderPassDesc& SetName(const std::string &name) { this->name = name; return *this; }
        const std::string& GetName() const { return name; }

        RenderPassDesc& AddColorAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                           VkAttachmentLoadOp load, VkAttachmentStoreOp store,
                                           VkImageLayout initialLayout, VkImageLayout finalLayout);

        RenderPassDesc& AddDepthAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                           VkAttachmentLoadOp load, VkAttachmentStoreOp store,
                                           VkImageLayout initialLayout, VkImageLayout finalLayout);

        RenderPassDesc& AddStencilAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                             VkAttachmentLoadOp load, VkAttachmentStoreOp store,
                                             VkImageLayout initialLayout, VkImageLayout finalLayout);

        RenderPassDesc& AddDepthStencilAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                                  VkAttachmentLoadOp load, VkAttachmentStoreOp store,
                                                  VkImageLayout initialLayout, VkImageLayout finalLayout);

        RenderPassDesc& AddResolveAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                             VkAttachmentLoadOp load, VkAttachmentStoreOp store,
                                             VkImageLayout initialLayout, VkImageLayout finalLayout);

    private:
        std::string name;
        std::vector<VkAttachmentDescription> attachments;
        std::vector<VkAttachmentReference> colorAttachments;
        std::vector<VkAttachmentReference> depthStencilAttachments;
        std::vector<VkAttachmentReference> resolveAttachments;
    };

    class RenderPass final : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkRenderPass> {
    public:
        RenderPass(Device *device, const RenderPassDesc &desc);
        virtual ~RenderPass();

    private:
        SmartPtr<Device> device;
    };

} // end of namespace slim

#endif // end of SLIM_CORE_RENDERPASS_H
