#include "core/query.h"

using namespace slim;

QueryPool::QueryPool(Device* device, VkQueryType queryType, uint32_t queryCount, VkQueryPoolCreateFlags flags) : device(device) {
    VkQueryPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    createInfo.queryType = queryType;
    createInfo.queryCount = queryCount;
    createInfo.flags = flags;
    ErrorCheck(DeviceDispatch(vkCreateQueryPool(*device, &createInfo, nullptr, &handle)), "create query pool");
}

QueryPool::~QueryPool() {
    if (handle) {
        DeviceDispatch(vkDestroyQueryPool(*device, handle, nullptr));
        handle = VK_NULL_HANDLE;
    }
}
