#ifndef SLIM_UTILITY_MESH_RENDERER_H
#define SLIM_UTILITY_MESH_RENDERER_H

#include <map>
#include <vector>

#include "utility/interface.h"
#include "utility/culling.h"

namespace slim {

    class MeshRenderer : public NotCopyable, public NotMovable, public ReferenceCountable {
    public:
        explicit MeshRenderer(const RenderInfo &info);
        virtual ~MeshRenderer();

        void Draw(Camera *camera, SceneFilter* filter, uint32_t firstQueue, uint32_t lastQueue);

    private:
        RenderInfo info;
    }; // end of MeshRenderer

} // end of namespace slim

#endif // SLIM_UTILITY_MESH_RENDERER_H
