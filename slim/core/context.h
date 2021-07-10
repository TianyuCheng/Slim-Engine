#ifndef SLIM_CORE_CONTEXT_H
#define SLIM_CORE_CONTEXT_H

#include <string>
#include <vector>
#include <functional>
#include <vulkan/vulkan.h>
#include <VmaUsage.h>

#include "core/vkutils.h"
#include "utility/interface.h"

namespace slim {

    class Window;
    class WindowDesc;
    class RenderFrame;
    class CommandBuffer;
    class DearImGui;

    // ContextDesc should hold the data for all configurations needed for context.
    // When context is initialized, nothing should be changeable.
    class ContextDesc final {
        friend class Context;
    public:
        ContextDesc& EnableValidation(bool value);
        ContextDesc& EnableGraphics(bool value);
        ContextDesc& EnableCompute(bool value);
    private:
        bool validation = false;
        bool graphics = true;
        bool compute = false;
        mutable bool present = false;
    };

    /**
     * Context will take care of the following things:
     * 1. vulkan instance creation
     * 2. vulkan physical selection / device creation
     * 3. vulkan memory allocator
     * 4. window creation (optional, if WindowDesc is provided)
     * 6. swapchain creation (optional, if WindowDesc is provided)
     **/
    class Context final : public NotCopyable, public NotMovable, public ReferenceCountable {
        friend class Window;
        friend class DearImGui;
        friend class RenderFrame;
    public:
        explicit Context(const ContextDesc &);
        explicit Context(const ContextDesc &, const WindowDesc &desc);
        virtual ~Context();

        Window*            GetWindow() const;
        VkDevice           GetDevice() const;
        VmaAllocator       GetMemoryAllocator() const;
        QueueFamilyIndices GetQueueFamilyIndices() const;
        void               WaitIdle() const;
        void               Execute(std::function<void(CommandBuffer*)> callback,
                                   VkQueueFlagBits queue = VK_QUEUE_TRANSFER_BIT);
        void               Execute(std::function<void(RenderFrame *, CommandBuffer*)> callback,
                                   VkQueueFlagBits queue = VK_QUEUE_TRANSFER_BIT);

    private:
        void    PrepareForWindow();
        void    PrepareForSwapchain();
        void    PrepareForViewport();
        void    PrepareForValidation();
        void    InitInstance(const ContextDesc &desc);
        void    InitDebuggerMessener(const ContextDesc &desc);
        void    InitSurface(const ContextDesc &desc);
        void    InitPhysicalDevice(const ContextDesc &desc);
        void    InitLogicalDevice(const ContextDesc &desc);
        void    InitMemoryAllocator(const ContextDesc &desc);

    private:
        VkInstance               instance       = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR             surface        = VK_NULL_HANDLE;
        VkPhysicalDevice         physicalDevice = VK_NULL_HANDLE;
        VkDevice                 device         = VK_NULL_HANDLE;
        VmaAllocator             allocator      = VK_NULL_HANDLE;
        Window*                  window         = nullptr;

        // device queues
        QueueFamilyIndices         queueFamilyIndices;
        VkQueue                    graphicsQueue         = VK_NULL_HANDLE;
        VkQueue                    computeQueue          = VK_NULL_HANDLE;
        VkQueue                    presentQueue          = VK_NULL_HANDLE;

        // extensions & layers
        std::vector<const char*>    validationLayers     = {};
        std::vector<const char*>    instanceExtensions   = {};
        std::vector<const char*>    deviceExtensions     = {};
    };

} // end of namespace slim

#endif // end of SLIM_CORE_CONTEXT_H
