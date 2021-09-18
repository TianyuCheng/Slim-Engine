#ifndef SLIM_UTILITY_MATERIAL_H
#define SLIM_UTILITY_MATERIAL_H

#include "core/shader.h"
#include "core/commands.h"
#include "core/pipeline.h"
#include "core/renderpass.h"
#include "core/renderframe.h"
#include "utility/camera.h"
#include "utility/technique.h"
#include "utility/interface.h"

namespace slim::scene {

    class Material final : public NotCopyable, public NotMovable, public ReferenceCountable {
    public:
        explicit Material(Device* device);
        explicit Material(Device* device, Technique* technique);
        virtual ~Material();

        void SetTechnique(Technique* technique);

        Technique* GetTechnique() const { return technique; }

        void SetTexture(const std::string &name, Image *texture, Sampler *sampler);
        void SetUniformBuffer(const std::string &name, Buffer *buffer, size_t offset = 0, size_t size = 0);
        void SetStorageBuffer(const std::string &name, Buffer *buffer, size_t offset = 0, size_t size = 0);

        template <typename T>
        void SetUniformBuffer(const std::string &name, const T &data) {
            UniformBuffer* uniform = uniformBufferPool->Request(sizeof(T));
            uniform->SetData(data);
            SetUniformBuffer(name, uniform, 0, 0);
        }

        uint32_t QueueIndex(RenderQueue queue) const {
            return technique->QueueIndex(queue);
        }

        PipelineLayout* Layout(uint32_t index) const {
            return technique->Layout(index);
        }

        void Bind(uint32_t queueIndex,
                  CommandBuffer *commandBuffer,
                  RenderFrame *renderFrame,
                  RenderPass *renderPass) const;

        template <typename T>
        T& GetData() { return *reinterpret_cast<T*>(data.data()); }

        template <typename T>
        void SetData(const T& value) {
            data.resize(sizeof(T));
            std::memcpy(data.data(), &value, sizeof(T));
        }

        // ID management
        bool HasID() const;
        void SetID(uint32_t id);
        void ResetID();
        uint32_t GetID() const;

    private:
        SmartPtr<Device> device;
        SmartPtr<Technique> technique = nullptr;
        SmartPtr<DescriptorPool> descriptorPool = nullptr;
        SmartPtr<BufferPool<UniformBuffer>> uniformBufferPool = nullptr;
        std::vector<SmartPtr<Descriptor>> descriptors;
        std::vector<uint8_t> data;
        int32_t materialId = -1;
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_MATERIAL_H
