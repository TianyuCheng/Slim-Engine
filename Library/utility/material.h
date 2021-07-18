#ifndef SLIM_UTILITY_MATERIAL_H
#define SLIM_UTILITY_MATERIAL_H

#include <functional>
#include "core/shader.h"
#include "core/commands.h"
#include "core/pipeline.h"
#include "core/renderpass.h"
#include "core/renderframe.h"
#include "utility/camera.h"
#include "utility/interface.h"

namespace slim {

    class SceneNode;
    class RenderPass;
    class RenderFrame;
    class CommandBuffer;
    class Material;

    struct RenderInfo {
        RenderPass*      renderPass     = nullptr;
        RenderFrame*     renderFrame    = nullptr;
        CommandBuffer*   commandBuffer  = nullptr;
        size_t           sceneNodeCount = 0;
        SceneNode**      sceneNodeData  = nullptr;
        SceneNode*       sceneNode      = nullptr;
        const Material*  material       = nullptr;
        const Camera*    camera         = nullptr;
    };

    class Material final : public ReferenceCountable {
    public:

        enum Queue : uint32_t {
            Opaque      = 1000,
            Background  = 2000,
            Unordered   = 3000,
            Transparent = 4000,
        };

        explicit Material(const std::string &name, Queue queue);
        virtual ~Material() = default;

        Queue GetMaterialQueue() { return queue; }

        GraphicsPipelineDesc& GetPipelineDesc() { return desc; }

        Pipeline* GetPipeline() const { return pipeline; }

        void Bind(const RenderInfo &renderInfo);

        std::function<void(const RenderInfo &)> PrepareMaterial
            = [](const RenderInfo &) { };

        std::function<void(const RenderInfo &)> PrepareSceneNode
            = [](const RenderInfo &) { };

    private:
        std::string name;
        Queue queue;
        GraphicsPipelineDesc desc;
        Pipeline* pipeline = nullptr;
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_MATERIAL_H
