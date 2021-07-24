#ifndef SLIM_UTILITY_RENDERQUEUE_H
#define SLIM_UTILITY_RENDERQUEUE_H

#include <cstdint>

namespace slim {

    // The items in the render queue references Unity's design:
    // https://docs.unity3d.com/ScriptReference/Rendering.RenderQueue.html
    enum RenderQueue : uint16_t {
        Background   = 0,
        Geometry     = 1000,
        AlphaTest    = 2000,
        GeometryLast = 3000,
        Transparent  = 4000,
        Overlay      = 5000,
        // --- name alias
        Opaque       = Geometry,
        OpaqueLast   = GeometryLast,
    };

    enum class SortingOrder {
        FrontToback,
        BackToFront,
        Unordered,
        // --- name alias
        Opaque       = FrontToback,
        Transparent  = BackToFront,
    };

    RenderQueue FindRenderQueue(uint32_t queue);

} // end of namespace slim

#endif // SLIM_UTILITY_RENDERQUEUE_H
