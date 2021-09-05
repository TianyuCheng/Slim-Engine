#include "core/debug.h"
#include "core/window.h"
#include "core/vkutils.h"

using namespace slim;

Semaphore::Semaphore(Device *device) : device(device) {
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.flags = 0;
    createInfo.pNext = nullptr;
    ErrorCheck(DeviceDispatch(vkCreateSemaphore(*device, &createInfo, nullptr, &handle)), "create semaphore");
}

Semaphore::~Semaphore() {
    if (handle) {
        vkDestroySemaphore(*device, handle, nullptr);
        handle = nullptr;
    }
}

Fence::Fence(Device *device, bool signaled) : device(device)  {
    VkFenceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0x0;
    createInfo.pNext = nullptr;
    ErrorCheck(DeviceDispatch(vkCreateFence(*device, &createInfo, nullptr, &handle)), "create fence");
}

Fence::~Fence() {
    if (handle) {
        vkDestroyFence(*device, handle, nullptr);
        handle = nullptr;
    }
}

void Fence::SetName(const std::string& name) const {
    if (device->IsDebuggerEnabled()) {
        VkDebugMarkerObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT;
        nameInfo.object = (uint64_t) handle;
        nameInfo.pObjectName = name.c_str();
        ErrorCheck(DeviceDispatch(vkDebugMarkerSetObjectNameEXT(*device, &nameInfo)), "set fence name");
    }
}

void Fence::Reset() const {
    ErrorCheck(vkResetFences(*device, 1, &handle), "reset fence");
}

void Fence::Wait(uint64_t timeout) const {
    ErrorCheck(vkWaitForFences(*device, 1, &handle, VK_TRUE, timeout), "wait for fence");
}

Event::Event(Device *device, bool deviceOnly) : device(device)  {
    VkEventCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    createInfo.flags = deviceOnly ? VK_EVENT_CREATE_DEVICE_ONLY_BIT_KHR : 0;
    createInfo.pNext = nullptr;
    ErrorCheck(DeviceDispatch(vkCreateEvent(*device, &createInfo, nullptr, &handle)), "create event");
}

Event::~Event() {
    if (handle) {
        vkDestroyEvent(*device, handle, nullptr);
        handle = nullptr;
    }
}

void Event::SetName(const std::string& name) const {
    if (device->IsDebuggerEnabled()) {
        VkDebugMarkerObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT;
        nameInfo.object = (uint64_t) handle;
        nameInfo.pObjectName = name.c_str();
        ErrorCheck(DeviceDispatch(vkDebugMarkerSetObjectNameEXT(*device, &nameInfo)), "set event name");
    }
}
