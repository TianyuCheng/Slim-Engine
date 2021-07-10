#include "core/debug.h"
#include "core/buffer.h"
#include "core/vkutils.h"

using namespace slim;

Buffer::Buffer(Context *context, size_t size, VkBufferUsageFlagBits bufferUsage, VmaMemoryUsage memoryUsage)
    : allocator(context->GetMemoryAllocator()), size(size) {

    if (size == 0) throw std::runtime_error("[Buffer] size should not be 0!");

    // create buffer
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = bufferUsage
                           | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
                           | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    // allocate memory
    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = memoryUsage;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    // use vulkan memory allocator
    ErrorCheck(vmaCreateBuffer(allocator, &bufferCreateInfo, &allocCreateInfo, &handle, &allocation, &allocInfo),
        "create buffer");
}

Buffer::~Buffer() {
    if (handle) {
        vmaDestroyBuffer(allocator, handle, allocation);
        handle = VK_NULL_HANDLE;
    }
}

bool Buffer::HostVisible() const {
    return allocInfo.pMappedData != nullptr;
}

size_t Buffer::Size() const {
    return allocInfo.size;
}

void Buffer::SetData(void *data, size_t size, size_t offset) const {
    // check if empty
    if (size == 0)
        throw std::runtime_error("[Buffer::SetData] input size == 0");

    // check against buffer size
    if (offset + size > this->size)
        throw std::runtime_error("[Buffer::SetData] offset + size > buffer size");

    // for host-mapped buffer
    if (char *dst = static_cast<char*>(allocInfo.pMappedData)) {
        char *src = const_cast<char*>(static_cast<const char*>(data));
        memcpy(dst + offset, src, size);
    }

    // for non host-mapped buffer
    else {
        throw std::runtime_error("Fail to set data on a device buffer!");
    }
}

void Buffer::Flush() const {
    vmaFlushAllocation(allocator, allocation, 0, size);
}
