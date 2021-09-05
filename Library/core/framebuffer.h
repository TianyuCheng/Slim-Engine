#ifndef SLIM_CORE_FRAMEBUFFER_H
#define SLIM_CORE_FRAMEBUFFER_H

#include <string>
#include <vector>

#include "core/vulkan.h"
#include "core/hasher.h"
#include "core/device.h"
#include "core/renderpass.h"
#include "utility/interface.h"

namespace slim {

    class FramebufferDesc final : public TriviallyConvertible<VkFramebufferCreateInfo> {
        friend class Framebuffer;
        friend class std::hash<FramebufferDesc>;
    public:
        explicit FramebufferDesc();
        virtual ~FramebufferDesc() = default;
        FramebufferDesc& SetExtent(uint32_t width, uint32_t height);
        FramebufferDesc& SetLayers(uint32_t layers);
        FramebufferDesc& SetRenderPass(RenderPass *renderPass);
        FramebufferDesc& AddAttachment(VkImageView view);
    private:
        std::vector<VkImageView> attachments;
    };

    class Framebuffer final : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkFramebuffer> {
    public:
        Framebuffer(Device *device, FramebufferDesc &desc);
        virtual ~Framebuffer();

        void SetName(const std::string& name) const;
    private:
        SmartPtr<Device> device;
    };

} // end of namespace slim

namespace std {
    template <>
    struct hash<slim::FramebufferDesc> {
        size_t operator()(const slim::FramebufferDesc& obj) const {
            size_t hash = 0x0;
            hash = slim::HashCombine(hash, obj.handle.width);
            hash = slim::HashCombine(hash, obj.handle.height);
            hash = slim::HashCombine(hash, obj.handle.layers);
            hash = slim::HashCombine(hash, obj.handle.renderPass);
            for (VkImageView view : obj.attachments) {
                hash = slim::HashCombine(hash, view);
            }
            return hash;
        }
    };
}

#endif // end of SLIM_CORE_FRAMEBUFFER_H
