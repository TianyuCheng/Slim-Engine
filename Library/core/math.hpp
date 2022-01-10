// This header file is intended to replace all includes for <cmath>, and <glm/glm.hpp>.
// <cmath> causes issues across different build systems with some macro definitions.
// <glm/glm.hpp> requires GLM_FORCE_RADIANS to behave consistently across the project.

#ifndef SLIM_CORE_MATH_H
#define SLIM_CORE_MATH_H

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif

#include <cmath>
#include <glm/glm.hpp>

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2  1.57079632679489661923
#endif

#endif // SLIM_CORE_MATH_H
