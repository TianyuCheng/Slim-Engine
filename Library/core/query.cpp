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

void QueryPool::SetName(const std::string& name) const {
    if (device->IsDebuggerEnabled()) {
        VkDebugMarkerObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT;
        nameInfo.object = (uint64_t) handle;
        nameInfo.pObjectName = name.c_str();
        ErrorCheck(DeviceDispatch(vkDebugMarkerSetObjectNameEXT(*device, &nameInfo)), "set query pool name");
    }
}
