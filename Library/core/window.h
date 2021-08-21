#ifndef SLIM_CORE_WINDOW_H
#define SLIM_CORE_WINDOW_H

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/image.h"
#include "core/context.h"
#include "core/device.h"
#include "core/renderframe.h"
#include "core/synchronization.h"
#include "utility/interface.h"

namespace slim {

    class Image;
    class Context;
    class DearImGui;

    // WindowDesc should hold the data for all configurations needed for window.
    // When window is initialized, nothing should be changeable (except for resolution)
    class WindowDesc final {
        friend class Window;
    public:
        WindowDesc& SetResolution(uint32_t width, uint32_t height);
        WindowDesc& SetFullScreen(bool value = true);
        WindowDesc& SetResizable(bool value = true);
        WindowDesc& SetAlphaComposite(bool value = true);
        WindowDesc& SetMaxFramesInFlight(uint32_t value);
        WindowDesc& SetMaxSetsPerPool(uint32_t value);
        WindowDesc& SetTitle(const std::string &title);

    private:
        bool        fullscreen = false;
        bool        resizable = false;
        bool        alphaComposite = false;
        uint32_t    width = 960;
        uint32_t    height = 720;
        uint32_t    maxFramesInFlight = 3;
        uint32_t    maxSetsPerPool = 256;
        std::string title = "Slim Engine";
    };

    /**
     * Window will take care of the following things:
     * 1. window creation
     * 2. swapchain format / present mode query
     * 3. create swapchain and handle swapchain recreation
     **/
    class Window final : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<GLFWwindow*> {
        friend class DearImGui;
        friend class RenderFrame;
    public:
        explicit Window(Device *device, const WindowDesc & desc);
        virtual ~Window();

        bool                               IsRunning();
        static void                        PollEvents();

        RenderFrame*                       AcquireNext();
        VkExtent2D                         GetExtent() const { return windowExtent; }
        VkFormat                           GetFormat() const { return swapchainFormat; }
        VkColorSpaceKHR                    GetColorSpace() const { return swapchainColorSpace; }
        GLFWwindow*                        GetWindow() const { return handle; }

    private:
        void                               InitSwapchain();
        void                               InitRenderFrames();
        void                               OnResize();

    private:
        WindowDesc                         desc;
        SmartPtr<Device>                   device              = nullptr;
        VkSurfaceKHR                       surface             = VK_NULL_HANDLE;
        VkExtent2D                         windowExtent;

        // swapchain
        VkFormat                           swapchainFormat;
        VkColorSpaceKHR                    swapchainColorSpace;
        VkExtent2D                         swapchainExtent;
        VkSwapchainKHR                     swapchain           = VK_NULL_HANDLE;
        std::vector<SmartPtr<GPUImage>>    swapchainImages     = {};

        // render/compute frames
        uint32_t                           currentFrame        = 0;
        uint32_t                           maxFramesInFlight   = 1;
        std::vector<SmartPtr<Fence>>       inflightFences      = {};
        std::vector<SmartPtr<RenderFrame>> renderFrames        = {};
    };

} // end of namespace slim

#endif // end of SLIM_CORE_WINDOW_H
