#include "core/debug.h"
#include "core/window.h"
#include "core/vkutils.h"

using namespace slim;

Semaphore::Semaphore(Context *context) : context(context) {
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.flags = 0;
    createInfo.pNext = nullptr;
    ErrorCheck(vkCreateSemaphore(context->GetDevice(), &createInfo, nullptr, &handle), "create semaphore");
}

Semaphore::~Semaphore() {
    if (handle) {
        vkDestroySemaphore(context->GetDevice(), handle, nullptr);
        handle = nullptr;
    }
}

Fence::Fence(Context *context, bool signaled) : context(context)  {
    VkFenceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0x0;
    createInfo.pNext = nullptr;
    ErrorCheck(vkCreateFence(context->GetDevice(), &createInfo, nullptr, &handle), "create fence");
}

Fence::~Fence() {
    if (handle) {
        vkDestroyFence(context->GetDevice(), handle, nullptr);
        handle = nullptr;
    }
}

void Fence::Reset() const {
    ErrorCheck(vkResetFences(context->GetDevice(), 1, &handle), "reset fence");
}

void Fence::Wait(uint64_t timeout) const {
    ErrorCheck(vkWaitForFences(context->GetDevice(), 1, &handle, VK_TRUE, timeout), "wait for fence");
}

Event::Event(Context *context, bool deviceOnly) : context(context)  {
    VkEventCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    createInfo.flags = deviceOnly ? VK_EVENT_CREATE_DEVICE_ONLY_BIT_KHR : 0;
    createInfo.pNext = nullptr;
    ErrorCheck(vkCreateEvent(context->GetDevice(), &createInfo, nullptr, &handle), "create event");
}

Event::~Event() {
    if (handle) {
        vkDestroyEvent(context->GetDevice(), handle, nullptr);
        handle = nullptr;
    }
}
