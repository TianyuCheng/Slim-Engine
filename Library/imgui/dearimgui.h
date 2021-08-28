#ifndef SLIM_IMGUI_DEARIMGUI_H
#define SLIM_IMGUI_DEARIMGUI_H

#include <string>
#include <vector>
#include <functional>

#include "imgui.h"
#include "core/vulkan.h"
#include "core/device.h"
#include "core/window.h"
#include "core/commands.h"
#include "utility/interface.h"

namespace slim {

    // GUI is responsible for storing frame-scoped data for rendering purpose.
    class DearImGui final : public NotCopyable, public NotMovable, public ReferenceCountable {
    public:
        using ImGuiInitFunction = std::function<void()>;

        explicit DearImGui(Device *device, Window *window, ImGuiInitFunction init = [](){ });
        virtual ~DearImGui();

        void Begin() const;
        void End() const;
        void Draw(CommandBuffer *commandBuffer) const;

        // imgui control / ui / etc
        static void EnableDocking();
        static void EnableMultiview();

    private:
        void InitImGui();
        void InitDescriptorPool();
        void InitRenderPass();
        void InitGlfw();
        void InitVulkan();
        void InitFontAtlas();

    private:
        SmartPtr<Device> device = nullptr;
        SmartPtr<Window> window = nullptr;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    };

} // end of namespace slim

#endif // end of SLIM_IMGUI_DEARIMGUI_H
