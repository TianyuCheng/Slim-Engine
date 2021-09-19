#ifndef SLIM_CORE_RENDERPASS_H
#define SLIM_CORE_RENDERPASS_H

#include <string>
#include <vector>
#include <deque>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

#include "core/vulkan.h"
#include "core/device.h"
#include "utility/interface.h"

namespace slim {

    class RenderPassDesc;

    class ClearValue final : public TriviallyConvertible<VkClearValue> {
    public:
        ClearValue(float r, float g, float b, float a = 1.0f);
        ClearValue(float depth, uint32_t stencil = 0U);
        ~ClearValue() = default;
    };

    inline bool IsDepthStencil(VkFormat format) {
        return (format == VK_FORMAT_D16_UNORM)
            || (format == VK_FORMAT_D16_UNORM_S8_UINT)
            || (format == VK_FORMAT_D24_UNORM_S8_UINT)
            || (format == VK_FORMAT_D32_SFLOAT)
            || (format == VK_FORMAT_D32_SFLOAT_S8_UINT);
    }

    class SubpassDesc {
        friend class RenderPass;
    public:
        explicit SubpassDesc(RenderPassDesc* parent, uint32_t subpassIndex);
        virtual ~SubpassDesc() = default;

        SubpassDesc& AddColorAttachment(uint32_t attachment, VkImageLayout inLayout, VkImageLayout outLayout);
        SubpassDesc& AddDepthAttachment(uint32_t attachment, VkImageLayout inLayout, VkImageLayout outLayout);
        SubpassDesc& AddStencilAttachment(uint32_t attachment, VkImageLayout inLayout, VkImageLayout outLayout);
        SubpassDesc& AddDepthStencilAttachment(uint32_t attachment, VkImageLayout inLayout, VkImageLayout outLayout);
        SubpassDesc& AddResolveAttachment(uint32_t attachment, VkImageLayout inLayout, VkImageLayout outLayout);
        SubpassDesc& AddInputAttachment(uint32_t attachment, VkImageLayout inLayout, VkImageLayout outLayout);
        SubpassDesc& AddPreserveAttachment(uint32_t attachment);

    private:
        RenderPassDesc* parent;
        uint32_t subpassIndex;
        std::vector<VkAttachmentReference> colorAttachments = {};
        std::vector<VkAttachmentReference> depthStencilAttachments = {};
        std::vector<VkAttachmentReference> resolveAttachments = {};
        std::vector<VkAttachmentReference> inputAttachments = {};
        std::vector<uint32_t> preserveAttachments = {};
        std::unordered_map<uint32_t, std::tuple<VkImageLayout, VkImageLayout>> layoutTransitionMap;
    };

    // RenderPassDesc should hold the data for all configurations needed for renderpass.
    // When renderpass is initialized, nothing should be changeable.
    class RenderPassDesc final {
        friend class RenderPass;
        friend class SubpassDesc;
    public:
        RenderPassDesc& SetName(const std::string &name) { this->name = name; return *this; }
        const std::string& GetName() const { return name; }

        SubpassDesc& AddSubpass();

        uint32_t AddAttachment(VkFormat format, VkSampleCountFlagBits samples,
                               VkAttachmentLoadOp load, VkAttachmentStoreOp store,
                               VkAttachmentLoadOp stencilLoad, VkAttachmentStoreOp stencilStore);
        uint32_t AddColorAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                    VkAttachmentLoadOp load, VkAttachmentStoreOp store);
        uint32_t AddColorResolveAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                           VkAttachmentLoadOp load, VkAttachmentStoreOp store);
        uint32_t AddDepthAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                    VkAttachmentLoadOp load, VkAttachmentStoreOp store);
        uint32_t AddStencilAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                      VkAttachmentLoadOp load, VkAttachmentStoreOp store);
        uint32_t AddDepthStencilAttachment(VkFormat format, VkSampleCountFlagBits samples,
                                           VkAttachmentLoadOp load, VkAttachmentStoreOp store);

    private:
        std::string name;
        std::vector<SubpassDesc> subpasses;
        mutable std::vector<VkAttachmentDescription> attachments;
        std::vector<std::unordered_set<uint32_t>> subpassesReads;
        std::vector<std::unordered_set<uint32_t>> subpassesWrites;
    };

    class RenderPass final : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkRenderPass> {
    public:
        RenderPass(Device *device, const RenderPassDesc &desc);
        virtual ~RenderPass();

    private:
        void ResolveSingleSubpassDependencies(const RenderPassDesc& desc, std::vector<VkSubpassDependency>& dependencies);
        void ResolveMultiSubpassDependencies(const RenderPassDesc& desc, std::vector<VkSubpassDependency>& dependencies);

    private:
        SmartPtr<Device> device;
    };

} // end of namespace slim

#endif // end of SLIM_CORE_RENDERPASS_H
