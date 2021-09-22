#ifndef SLIM_EXAMPLE_LIGHT_H
#define SLIM_EXAMPLE_LIGHT_H

#include <slim/slim.hpp>

using namespace slim;

struct DirectionalLight {
    glm::vec4 direction;
    glm::vec4 color;
};

#endif // SLIM_EXAMPLE_LIGHT_H
