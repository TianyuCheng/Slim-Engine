#ifndef SLIM_UTILITY_CULLING_H
#define SLIM_UTILITY_CULLING_H

#include <map>
#include <vector>
#include <algorithm>

#include "utility/view.h"
#include "utility/mesh.h"
#include "utility/material.h"
#include "utility/technique.h"
#include "utility/interface.h"
#include "utility/scenegraph.h"

namespace slim {

    using DrawCommand = VkDrawIndirectCommand;
    using DrawIndexed = VkDrawIndexedIndirectCommand;
    using DrawVariant = std::variant<DrawCommand, DrawIndexed>;

    // data structure for objects to be drawn
    struct Drawable {
        SmartPtr<scene::Node>     node;
        SmartPtr<scene::Mesh>     mesh;
        SmartPtr<scene::Material> material;
        DrawVariant               drawCommand;
        RenderQueue               queue;
        float                     distanceToCamera;
    };

    // helper functions for sorting
    bool SortDrawableAscending(const Drawable& d1, const Drawable& d2);
    bool SortDrawableDescending(const Drawable& d1, const Drawable& d2);

    using RenderQueueMap = std::map<RenderQueue, std::vector<Drawable>>;

    class CPUCulling : public NotCopyable, public NotMovable, public ReferenceCountable {
    public:
        void Clear();
        void Cull(scene::Node* scene, Camera* camera);
        void Sort(uint32_t firstQueue, uint32_t lastQueue, SortingOrder sorting);

        View<Drawable> GetDrawables(uint32_t firstQueue, uint32_t lastQueue);

    private:
        bool CullSceneNode(scene::Node* scene, Camera* camera);

    private:
        RenderQueueMap objects;
    };

} // end of namespace slim

#endif // SLIM_UTILITY_CULLING_H
