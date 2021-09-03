#ifndef SLIM_UTILITY_TECHNIQUE_H
#define SLIM_UTILITY_TECHNIQUE_H

#include <string>
#include <unordered_map>
#include "core/pipeline.h"
#include "core/renderframe.h"
#include "utility/interface.h"
#include "utility/renderqueue.h"

namespace slim {

    class Technique final : public ReferenceCountable {
    public:

        struct Pass {
            GraphicsPipelineDesc desc;
            RenderQueue          queue;
            SmartPtr<Pipeline>   pipeline;
        };

        explicit Technique();
        virtual ~Technique();

        // adding shader pass
        void AddPass(RenderQueue queue, const GraphicsPipelineDesc &desc);

        // bind pipeline
        void Bind(uint32_t index,
                  RenderFrame *renderFrame,
                  RenderPass *renderPass,
                  CommandBuffer *commandBuffer);

        PipelineLayout* Layout(uint32_t index) const;

        uint32_t QueueIndex(RenderQueue queue) const;

        VkPipelineBindPoint Type(uint32_t index) const;

        // passes traversal
        auto begin()       { return passes.begin(); }
        auto end()         { return passes.end();   }
        auto begin() const { return passes.begin(); }
        auto end()   const { return passes.end();   }

    private:
        std::vector<Pass> passes;
        std::unordered_map<RenderQueue, uint32_t> queue2index;
    };

} // end of namespace slim

#endif // SLIM_UTILITY_TECHNIQUE_H
