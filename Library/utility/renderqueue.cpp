#include "utility/renderqueue.h"

namespace slim {

    RenderQueue FindRenderQueue(uint32_t queue) {
        if (queue >= Overlay)
            return Overlay;
        else if (queue >= Transparent)
            return Transparent;
        else if (queue >= GeometryLast)
            return GeometryLast;
        else if (queue >= AlphaTest)
            return AlphaTest;
        else if (queue >= Geometry)
            return Geometry;
        return Background;
    }

}
