#include "culling.h"

using namespace slim;

namespace slim {

    bool SortDrawableAscending(const Drawable& d1, const Drawable& d2) {
        return d1.distanceToCamera < d2.distanceToCamera;
    }

    bool SortDrawableDescending(const Drawable& d1, const Drawable& d2) {
        return d1.distanceToCamera > d2.distanceToCamera;
    }

}

void CPUCulling::Clear() {
    objects.clear();
}

void CPUCulling::Cull(Scene* scene, Camera* camera) {
    scene->ForEach([&](Scene* scene) {
        return CullSceneNode(scene, camera);
    });
}

bool CPUCulling::CullSceneNode(Scene* scene, Camera*) {

    // when user configures this object to be invisible
    if (!scene->IsVisible()) {
        return false;
    }

    // TODO: perform camera-based frustum culling
    bool visible = true;
    float distance = 0.0f;

    if (!visible) {
        // children objects are not visible either
        return false;
    }

    for (const Primitive& primitive : *scene) {
        // find technique
        Technique* technique = primitive.material->GetTechnique();

        // find queue for each pass
        for (auto &pass : *technique) {
            // queue
            auto queueIt = objects.find(pass.queue);
            if (queueIt == objects.end()) {
                objects.insert(std::make_pair(pass.queue, std::vector<Drawable>()));
                queueIt = objects.find(pass.queue);
            }
            // drawable
            queueIt->second.push_back(Drawable {
                scene,
                primitive.mesh, primitive.material, primitive.drawCommand,
                pass.queue,
                distance
            });
        }
    }

    return true;
}

void CPUCulling::Sort(uint32_t firstQueue, uint32_t lastQueue, SortingOrder sorting) {
    for (auto& kv : objects) {
        RenderQueue queue = kv.first;
        if (queue >= firstQueue && queue <= lastQueue) {
            switch (sorting) {
                case SortingOrder::FrontToback:
                    std::sort(kv.second.begin(), kv.second.end(), SortDrawableAscending);
                    break;
                case SortingOrder::BackToFront:
                    std::sort(kv.second.begin(), kv.second.end(), SortDrawableDescending);
                    break;
                default:
                    break;
            }
        }
    }
}

View<Drawable> CPUCulling::GetDrawables(uint32_t firstQueue, uint32_t lastQueue) {
    View<Drawable> drawables;
    for (auto& kv : objects) {
        RenderQueue queue = kv.first;
        if (queue >= firstQueue && queue <= lastQueue) {
            drawables.Concat(kv.second.begin(), kv.second.end());
        }
    }
    return drawables;
}
