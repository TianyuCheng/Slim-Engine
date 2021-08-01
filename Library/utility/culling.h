#ifndef SLIM_UTILITY_CULLING_H
#define SLIM_UTILITY_CULLING_H

#include <map>
#include <vector>

#include "utility/mesh.h"
#include "utility/material.h"
#include "utility/technique.h"
#include "utility/interface.h"
#include "utility/scenegraph.h"

namespace slim {

    struct Drawable {
        Scene* node = nullptr;
        float distanceToCamera = 0.0;
    };

    using RenderQueueMap = std::map<RenderQueue, std::vector<Drawable>>;

    class SceneFilter : public NotCopyable, public NotMovable, public ReferenceCountable {
    public:
        void Cull(Scene* scene, Camera* camera);
        void Sort(uint32_t firstQueue, uint32_t lastQueue, SortingOrder sorting);
        void Draw(uint32_t firstQueue, uint32_t lastQueue);

    private:
        bool CullSceneNode(Scene* scene, Camera* camera);

    public:
        RenderQueueMap objects;
    };

} // end of namespace slim

#endif // SLIM_UTILITY_CULLING_H
