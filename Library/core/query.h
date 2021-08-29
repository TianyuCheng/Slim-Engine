#ifndef SLIM_CORE_QUERY_H
#define SLIM_CORE_QUERY_H

#include "core/vulkan.h"
#include "core/debug.h"
#include "core/device.h"
#include "core/vkutils.h"
#include "utility/interface.h"

namespace slim {

    class QueryPool : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkQueryPool> {
    public:
        explicit QueryPool(Device* device, VkQueryType queryType, uint32_t queryCount, VkQueryPoolCreateFlags flags);
        virtual ~QueryPool();

    private:
        SmartPtr<Device> device;
    };

} // end of namespace slim

#endif // SLIM_CORE_QUERY_H
