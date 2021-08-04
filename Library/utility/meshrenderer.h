#ifndef SLIM_UTILITY_MESH_RENDERER_H
#define SLIM_UTILITY_MESH_RENDERER_H

#include <map>
#include <vector>

#include "utility/view.h"
#include "utility/camera.h"
#include "utility/culling.h"
#include "utility/interface.h"
#include "utility/rendergraph.h"

namespace slim {

    class MeshRenderer : public NotCopyable, public NotMovable, public ReferenceCountable {
    public:

        // for scene camera setup
        struct CameraData {
            alignas(16) glm::mat4 view;
            alignas(16) glm::mat4 proj;
        };

        // for object transform setup
        struct alignas(256) ModelData {
            alignas(16) glm::mat4 model;
            alignas(16) glm::mat3 normal;
        };

        explicit MeshRenderer(const RenderInfo &info);
        virtual ~MeshRenderer();

        void Draw(Camera *camera, const View<Drawable>& drawables);

    private:
        RenderInfo info;
    }; // end of MeshRenderer

} // end of namespace slim

#endif // SLIM_UTILITY_MESH_RENDERER_H
