#ifndef SLIM_CORE_BUFFER_H
#define SLIM_CORE_BUFFER_H

#include <list>
#include <vector>
#include <string>
#include <VmaUsage.h>
#include <unordered_map>
#include <vulkan/vulkan.h>

#include "core/context.h"
#include "utility/transient.h"
#include "utility/interface.h"

namespace slim {

    template <typename T>
    size_t BufferSize(const std::vector<T> &data) {
        return sizeof(T) * data.size();
    }

    template <typename Buffer>
    class BufferPool final {
        using List = std::list<SmartPtr<Buffer>>;
    public:
        explicit BufferPool(Context *context);
        virtual ~BufferPool();
        void Reset();
        Buffer* Request(size_t size);
    private:
        Buffer* AllocateBuffer(size_t size);
    private:
        SmartPtr<Context> context;
        List allAllocations;
    };

    template <typename Buffer>
    BufferPool<Buffer>::BufferPool(Context *context) : context(context) {
    }

    template <typename Buffer>
    BufferPool<Buffer>::~BufferPool() {
        allAllocations.clear();
    }

    template <typename Buffer>
    void BufferPool<Buffer>::Reset() {
        allAllocations.clear();
    }

    template <typename Buffer>
    Buffer* BufferPool<Buffer>::Request(size_t size) {
        return AllocateBuffer(size);
    }

    template <typename Buffer>
    Buffer* BufferPool<Buffer>::AllocateBuffer(size_t size) {
        Buffer *buffer = new Buffer(context.get(), size);
        allAllocations.push_back(SmartPtr<Buffer>(buffer));
        return buffer;
    }

    // --------------------------------------------------------

    class Buffer : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkBuffer> {
    public:
        explicit Buffer(Context *context, size_t size, VkBufferUsageFlagBits bufferUsage, VmaMemoryUsage memoryUsage);
        virtual ~Buffer();

        void SetData(void *data, size_t size, size_t offset = 0) const;

        void Flush() const;

        bool HostVisible() const;

        size_t Size() const;

        template <typename T>
        void SetData(const T &data);

        template <typename T>
        void SetData(const std::vector<T> &data);

        template <typename T>
        T* GetData(size_t offset = 0) const;

        template <typename T>
        size_t Size(size_t offset = 0) const;

    private:
        VmaAllocator      allocator = VK_NULL_HANDLE;
        VmaAllocation     allocation;
        VmaAllocationInfo allocInfo;
        size_t            size;
    };

    template <typename T>
    void Buffer::SetData(const T &data) {
        SetData(const_cast<T*>(&data), sizeof(data));
    }

    template <typename T>
    void Buffer::SetData(const std::vector<T> &data) {
        SetData(const_cast<T*>(data.data()), data.size() * sizeof(T));
    }

    template <typename T>
    T* Buffer::GetData(size_t offset) const {
        #ifndef NDEBUG
        if (!HostVisible())
            throw std::runtime_error("This buffer is not mapped to host!");
        #endif
        return static_cast<T*>(allocInfo.pMappedData) + offset;
    }

    template <typename T>
    size_t Buffer::Size(size_t offset) const {
        return (size - offset) / sizeof(T);
    }

    #define BUFFER_TYPE(NAME, BUFFER_USAGE, MEMORY_USAGE)         \
    class NAME final : public Buffer {                            \
    public:                                                       \
        friend class BufferPool<NAME>;                            \
        using PoolType = BufferPool<NAME>;                        \
        NAME(Context *context, size_t size)                       \
            : Buffer(context, size, BUFFER_USAGE, MEMORY_USAGE) { \
        }                                                         \
        virtual ~NAME() { }                                       \
    };

    BUFFER_TYPE(StagingBuffer,       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,   VMA_MEMORY_USAGE_CPU_ONLY);
    BUFFER_TYPE(VertexBuffer,        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,  VMA_MEMORY_USAGE_GPU_ONLY);
    BUFFER_TYPE(UniformBuffer,       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    BUFFER_TYPE(HostStorageBuffer,   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);  // TODO: need to think more about the memory usage
    BUFFER_TYPE(DeviceStorageBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);  // TODO: need to think more about the memory usage
    #undef BUFFER_TYPE

    class IndexBuffer final : public Buffer {
        friend class CommandBuffer;
    public:
        friend class BufferPool<IndexBuffer>;
        using PoolType = BufferPool<IndexBuffer>;

        IndexBuffer(Context *context, size_t size)
            : Buffer(context, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY) {}

        virtual ~IndexBuffer() { }

        void SetData(const std::vector<uint16_t> &data) {
            Buffer::SetData<uint16_t>(data);
            indexType = VK_INDEX_TYPE_UINT16;
        }

        void SetData(const std::vector<uint32_t> &data) {
            Buffer::SetData<uint32_t>(data);
            indexType = VK_INDEX_TYPE_UINT32;
        }

    private:
        VkIndexType indexType = VK_INDEX_TYPE_UINT32;
    };

} // end of namespace slim

#endif // end of SLIM_CORE_BUFFER_H
