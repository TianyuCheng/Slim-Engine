#ifndef SLIM_SHADER_LIB_GLSL_HPP
#define SLIM_SHADER_LIB_GLSL_HPP

#ifdef __cplusplus
#define GLM_FORCE_RADIANS
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <glm/glm.hpp>

using namespace std;
using namespace glm;

using uint    = uint32_t;
using uvec2   = glm::uvec2;
using uvec3   = glm::uvec3;
using uvec4   = glm::uvec4;

using vec2    = glm::vec2;
using vec3    = glm::vec3;
using vec4    = glm::vec4;

using mat2    = glm::mat2;
using mat3    = glm::mat3;
using mat4    = glm::mat4;

#define SLIM_ATTR inline
#define MEDIUMP_FLT_MAX    65504.0f
#define saturate_mediump(x) std::min(x, MEDIUMP_FLT_MAX)
#define highp
#define mediump
#define lowp
#define max(a, b)  ((a) > (b) ? (a) : (b))
#define min(a, b)  ((a) < (b) ? (a) : (b))

#else

#define SLIM_ATTR
#define MEDIUMP_FLT_MAX    65504.0
#define saturate_mediump(x) min(x, MEDIUMP_FLT_MAX)

#endif

const float PI = 3.14159265357;

#endif // SLIM_SHADER_LIB_GLSL_HPP
