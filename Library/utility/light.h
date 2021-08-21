#ifndef SLIM_UTILITY_LIGHT_H
#define SLIM_UTILITY_LIGHT_H

#include <glm/glm.hpp>

namespace slim {

    struct LightInfo {
        alignas(16) glm::vec3 color;
        float intensity;
        float distance = 0.0; // maximum range of the light, 0 for no limit
        float decay = 1.0;    // the amount of light dims along the distance of the light, 2 for physically correct lighting
    };

    struct PointLight {
        alignas(16) glm::vec3 position;
        LightInfo light;
    };

    struct DirectionalLight {
        alignas(16) glm::vec3 direction;
        LightInfo light;
    };

    struct SpotLight {
        alignas(16) glm::vec3 position;
        alignas(16) glm::vec3 direction;
        float angle;
        float penumbra;
        LightInfo light;
    };

} // end of namespace slim

#endif // SLIM_UTILITY_LIGHT_H
