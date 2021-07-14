#ifndef SLIM_HPP
#define SLIM_HPP

// core
#include "core/buffer.h"
#include "core/commands.h"
#include "core/context.h"
#include "core/image.h"
#include "core/pipeline.h"
#include "core/renderpass.h"
#include "core/renderframe.h"
#include "core/shader.h"
#include "core/window.h"

// utility
#include "utility/stb.h"
#include "utility/color.h"
#include "utility/texture.h"
#include "utility/camera.h"
#include "utility/transform.h"
#include "utility/scenegraph.h"
#include "utility/rendergraph.h"

// third party
#include <imgui.h>
#include <imnodes.h>
#include "imgui/dearimgui.h"
#include "imgui/imgui_impl_vulkan.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

#endif // SLIM_HPP
