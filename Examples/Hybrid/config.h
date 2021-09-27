#ifndef SLIM_EXAMPLE_CONFIG_H
#define SLIM_EXAMPLE_CONFIG_H

#include <cstdint>

#define ENABLE_RAY_TRACING
constexpr uint32_t MAX_NUM_SURFELS       = 250000;
constexpr uint32_t NUM_SURFELS_ON_WIDTH  = 500;
constexpr uint32_t NUM_SURFELS_ON_HEIGHT = 500;
constexpr uint32_t SURFEL_MINIMAP_WIDTH  = 6;
constexpr uint32_t SURFEL_MINIMAP_HEIGHT = 6;

#endif // SLIM_EXAMPLE_CONFIG_H
