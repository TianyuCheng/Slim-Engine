#ifndef SLIM_IMGUI_DEARIMGUI_H
#define SLIM_IMGUI_DEARIMGUI_H

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "core/context.h"
#include "core/commands.h"
#include "utility/interface.h"

namespace slim {

    // GUI is responsible for storing frame-scoped data for rendering purpose.
    class DearImGui final : public NotCopyable, public NotMovable, public ReferenceCountable {
    public:
        explicit DearImGui(Context *context);
        explicit DearImGui(Context *context, VkImage image);
        virtual ~DearImGui();

        void Begin() const;
        void End() const;
        void Draw(CommandBuffer *commandBuffer) const;

    private:
        void InitImGui();
        void InitDescriptorPool();
        void InitRenderPass();
        void InitGlfw();
        void InitVulkan();
        void InitFontAtlas();

    private:
        Context* context = nullptr;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    };

} // end of namespace slim

#endif // end of SLIM_IMGUI_DEARIMGUI_H
