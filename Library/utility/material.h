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

namespace slim {

    class Material final : public NotCopyable, public NotMovable, public ReferenceCountable {
    public:
        explicit Material(Device *device, Technique *technique);
        virtual ~Material();

        Technique* GetTechnique() const { return technique; }

        void SetTexture(const std::string &name, Image *texture, Sampler *sampler);
        void SetUniform(const std::string &name, Buffer *buffer, size_t offset = 0, size_t size = 0);
        void SetStorage(const std::string &name, Buffer *buffer, size_t offset = 0, size_t size = 0);

        template <typename T>
        void SetUniform(const std::string &name, const T &data) {
            UniformBuffer* uniform = uniformBufferPool->Request(sizeof(T));
            uniform->SetData(data);
            SetUniform(name, uniform, 0, 0);
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

    private:
        SmartPtr<Device> device;
        SmartPtr<Technique> technique = nullptr;
        SmartPtr<DescriptorPool> descriptorPool = nullptr;
        SmartPtr<BufferPool<UniformBuffer>> uniformBufferPool = nullptr;
        std::vector<SmartPtr<Descriptor>> descriptors;
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_MATERIAL_H
