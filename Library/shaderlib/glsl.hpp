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
#define HIGHP_FLT_MAX      3.40282e+038f
#define saturate_mediump(x) std::min(x, MEDIUMP_FLT_MAX)
#define highp
#define mediump
#define lowp
#define max(a, b)  ((a) > (b) ? (a) : (b))
#define min(a, b)  ((a) < (b) ? (a) : (b))

#else

#define SLIM_ATTR
#define MEDIUMP_FLT_MAX    65504.0
#define HIGHP_FLT_MAX      3.40282e+038f
#define saturate_mediump(x) min(x, MEDIUMP_FLT_MAX)

#endif

// commonly used constants
const float PI      = 3.14159265358979323846;
const float InvPI   = 0.31830988618379067154;
const float Inv2PI  = 0.15915494309189533577;
const float Inv4PI  = 0.07957747154594766788;
const float PIOver2 = 1.57079632679489661923;
const float PIOver4 = 0.78539816339744830961;
const float Sqrt2   = 1.41421356237309504880;

#endif // SLIM_SHADER_LIB_GLSL_HPP
