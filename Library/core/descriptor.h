#ifndef SLIM_CORE_DESCRIPTOR_H
#define SLIM_CORE_DESCRIPTOR_H

#include <string>
#include "core/pipeline.h"

namespace slim {

    class CommandBuffer;

    //  ____                      _       _
    // |  _ \  ___  ___  ___ _ __(_)_ __ | |_ ___  _ __
    // | | | |/ _ \/ __|/ __| '__| | '_ \| __/ _ \| '__|
    // | |_| |  __/\__ \ (__| |  | | |_) | || (_) | |
    // |____/ \___||___/\___|_|  |_| .__/ \__\___/|_|
    //                             |_|

    class Descriptor final : public NotCopyable, public NotMovable, public ReferenceCountable {
        friend class CommandBuffer;
    public:
        explicit Descriptor(DescriptorPool* pool, PipelineLayout* pipelineLayout);
        virtual ~Descriptor();

        // binding a uniform buffer, with offset and size for the target buffer
        void SetUniform(const std::string& name, Buffer* buffer);
        void SetUniform(const std::string& name, const BufferAlloc& bufferAlloc);
        void SetUniforms(const std::string& name, const std::vector<BufferAlloc>& bufferAllocs);

        // binding a dynamic uniform buffer, with offset and size for the target buffer
        // NOTE: size is for each individual uniform element in the buffer
        void SetDynamic(const std::string& name, Buffer* buffer, size_t elemSize);
        void SetDynamic(const std::string& name, const BufferAlloc& bufferAlloc);

        // binding a storage buffer, with offset and size for the target buffer
        void SetStorage(const std::string& name, Buffer* buffer);
        void SetStorage(const std::string& name, const BufferAlloc& bufferAlloc);
        void SetStorages(const std::string& name, const std::vector<BufferAlloc>& bufferAllocs);

        // binding a combined image + sampler
        void SetTexture(const std::string& name, Image* image, Sampler* sampler);
        void SetTextures(const std::string& name, const std::vector<Image*>& images, const std::vector<Sampler*>& samplers);

        // binding image uniform
        void SetSampledImage(const std::string& name, Image* image);
        void SetSampledImages(const std::string& name, const std::vector<Image*>& images);

        // binding image uniform
        void SetStorageImage(const std::string& name, Image* image);
        void SetStorageImages(const std::string& name, const std::vector<Image*>& images);

        // binding sampler uniform
        void SetSampler(const std::string& name, Sampler* sampler);
        void SetSamplers(const std::string& name, const std::vector<Sampler*>& samplers);

        // binding acceleration structure
        void SetAccelStruct(const std::string& name, accel::AccelStruct* accel);
        void SetAccelStructs(const std::string& name, const std::vector<accel::AccelStruct*>& accels);

        bool HasBinding(const std::string &name) const;
        std::tuple<uint32_t, uint32_t> GetBinding(const std::string &name);

        void SetDynamicOffset(const std::string &name, uint32_t offset);
        void SetDynamicOffset(uint32_t set, uint32_t binding, uint32_t offset);
    private:
        void Update();
        std::tuple<uint32_t, uint32_t, VkDescriptorBindingFlags> FindDescriptorSet(const std::string &name);
        void SetBuffer(const std::string &name, VkDescriptorType descriptorType, const std::vector<BufferAlloc> &bufferAlloc);
    private:
        SmartPtr<DescriptorPool> pool;
        SmartPtr<PipelineLayout> pipelineLayout;
        std::vector<VkWriteDescriptorSet> writes;
        std::vector<uint32_t> writeDescriptorSets;
        std::unordered_map<uint32_t, uint32_t> variableDescriptorCounts;
        std::vector<VkDescriptorSet> descriptorSets;
        std::list<std::vector<VkDescriptorBufferInfo>> bufferInfos;
        std::list<std::vector<VkDescriptorImageInfo>> imageInfos;
        std::list<VkWriteDescriptorSetAccelerationStructureKHR> accelInfos;
        std::list<std::vector<VkAccelerationStructureKHR>> accelerations;
        std::vector<std::vector<uint32_t>> dynamicOffsets;
    };

    //  ____                      _       _             ____             _
    // |  _ \  ___  ___  ___ _ __(_)_ __ | |_ ___  _ __|  _ \ ___   ___ | |
    // | | | |/ _ \/ __|/ __| '__| | '_ \| __/ _ \| '__| |_) / _ \ / _ \| |
    // | |_| |  __/\__ \ (__| |  | | |_) | || (_) | |  |  __/ (_) | (_) | |
    // |____/ \___||___/\___|_|  |_| .__/ \__\___/|_|  |_|   \___/ \___/|_|
    //                             |_|

    class DescriptorPool final : public ReferenceCountable {
        friend class Descriptor;
    public:
        explicit DescriptorPool(Device *device, uint32_t poolSize);
        virtual ~DescriptorPool();
        void Reset();
        VkDescriptorSet Request(VkDescriptorSetLayout layout, uint32_t variableDescriptorCount = 0);
        Device* GetDevice() const { return device; }
    private:
        uint32_t FindAvailablePoolIndex(uint32_t poolIndex);
    private:
        SmartPtr<Device> device;
        uint32_t poolIndex = 0;
        uint32_t maxPoolSets;
        std::vector<VkDescriptorPool> pools;
        std::vector<VkDescriptorPoolSize> poolSizes;
        std::vector<uint32_t> poolSetCounts;
    };

}

#endif // SLIM_CORE_DESCRIPTOR_H
