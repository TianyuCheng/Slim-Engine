#ifndef SLIM_CORE_SYNCHRONIZATION_H
#define SLIM_CORE_SYNCHRONIZATION_H

#include <string>
#include <vector>

#include "core/vulkan.h"
#include "core/device.h"
#include "utility/interface.h"

namespace slim {

    // semaphores are a synchronization primitive that can be used to insert a dependency between
    // queue operations and the host.
    class Semaphore final : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkSemaphore> {
    public:
        explicit Semaphore(Device *device);
        virtual ~Semaphore();
        void SetName(const std::string& name) const;
    private:
        SmartPtr<Device> device = nullptr;
    };

    // fences are a synchronization primitive that can be used to insert a dependency from a queue to the host
    class Fence final : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkFence> {
    public:
        explicit Fence(Device *device, bool signaled = false);
        virtual ~Fence();
        void Reset() const;
        void Wait(uint64_t timeout = UINT64_MAX) const;
        void SetName(const std::string& name) const;
    private:
        SmartPtr<Device> device = nullptr;
    };

    // events are a synchronization primitive that can be used to insert a fine-grained dependency between
    // commands submitted to the same queue, or between two states - signaled and unsignaled.
    class Event final : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkEvent> {
    public:
        explicit Event(Device *device, bool deviceOnly = false);
        virtual ~Event();
        void SetName(const std::string& name) const;
    private:
        SmartPtr<Device> device = nullptr;
    };

} // end of namespace slim

#endif // end of SLIM_CORE_SYNCHRONIZATION_H
