#ifndef SLIM_CORE_DEVICE_H
#define SLIM_CORE_DEVICE_H

#include <string>
#include <vector>
#include <iostream>
#include <functional>

#include "core/vulkan.h"
#include "core/vkutils.h"
#include "core/context.h"
#include "utility/interface.h"

#define DeviceDispatch(CALL) (device->deviceTable.CALL)

namespace slim {

    class RenderFrame;
    class CommandBuffer;
    class Buffer;
    namespace accel {
        class AccelStruct;
    };

    /**
     * Device will take care of the following things:
     * 1. vulkan instance creation
     * 2. vulkan physical selection / device creation
     * 3. vulkan memory allocator
     * 4. window creation (optional, if WindowDesc is provided)
     * 6. swapchain creation (optional, if WindowDesc is provided)
     **/
    class Device final : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkDevice> {
    public:
        explicit Device(Context *context);
        virtual ~Device();

        void WaitIdle() const;
        void WaitIdle(VkQueueFlagBits queue) const;

        Context*           GetContext() const;
        VmaAllocator       GetMemoryAllocator() const;
        QueueFamilyIndices GetQueueFamilyIndices() const;
        void               Execute(std::function<void(CommandBuffer*)> callback,
                                   VkQueueFlagBits queue = VK_QUEUE_TRANSFER_BIT);
        void               Execute(std::function<void(RenderFrame *, CommandBuffer*)> callback,
                                   VkQueueFlagBits queue = VK_QUEUE_TRANSFER_BIT);

        VkQueue            GetComputeQueue() const { return computeQueue; }
        VkQueue            GetGraphicsQueue() const { return graphicsQueue; }
        VkQueue            GetPresentQueue() const { return presentQueue; }
        VkQueue            GetTransferQueue() const { return transferQueue; }

        VkDeviceAddress    GetDeviceAddress(Buffer* buffer) const;
        VkDeviceAddress    GetDeviceAddress(accel::AccelStruct* as) const;

        VolkDeviceTable    deviceTable;

    private:
        void InitLogicalDevice();
        void InitMemoryAllocator();

    private:
        SmartPtr<Context> context;

        VmaAllocator               allocator      = VK_NULL_HANDLE;

        // device queues
        QueueFamilyIndices         queueFamilyIndices;
        VkQueue                    graphicsQueue         = VK_NULL_HANDLE;
        VkQueue                    computeQueue          = VK_NULL_HANDLE;
        VkQueue                    presentQueue          = VK_NULL_HANDLE;
        VkQueue                    transferQueue         = VK_NULL_HANDLE;
    };

} // end of namespace slim

#endif // end of SLIM_CORE_CONTEXT_H
