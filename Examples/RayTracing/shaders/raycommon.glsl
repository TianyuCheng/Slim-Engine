const float PI = 3.1415926;
const float RAY_BIAS = 0.0001;

struct HitPayload
{
    vec3 position;
    vec3 direction;
    vec3 normal;
    vec3 color;
    vec3 fraction;
    bool terminate;
};

struct Vertex
{
  vec3 position;
  vec3 normal;
  vec2 texCoord;
};

struct Material {
    vec4 baseColor;
    vec4 emissiveColor;
};

uint step_rng(uint rngState) {
    return rngState * 747796405 + 1;
}

float random_float(inout uint rngState) {
    rngState = step_rng(rngState);
    uint word = ((rngState >> ((rngState >> 28) + 4)) ^ rngState) * 277803737;
    word = (word >> 22) ^ word;
    return float(word) / 4294967295.0;
}

vec3 uniform_sample_hemisphere(float u1, float u2) {
    float r = sqrt(1.0 - u1 * u1);
    float phi = 2 * PI * u2;
    vec3 dir = vec3(cos(phi) * r, sin(phi) * r, u1);
    return dir;
}

vec3 cosine_sample_hemisphere(float u1, float u2) {
	float r = sqrt(u1);
	float theta = 2 * PI * u2;
	float x = r * cos(theta);
	float y = r * sin(theta);
	vec3 dir = vec3(x, y, sqrt(max(0.0f, 1 - u1)));
    return dir;
}
