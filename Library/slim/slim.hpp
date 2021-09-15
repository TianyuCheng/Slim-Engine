#ifndef SLIM_HPP
#define SLIM_HPP

// core
#include "core/vulkan.h"
#include "core/context.h"
#include "core/device.h"
#include "core/buffer.h"
#include "core/commands.h"
#include "core/image.h"
#include "core/pipeline.h"
#include "core/renderpass.h"
#include "core/renderframe.h"
#include "core/shader.h"
#include "core/window.h"
#include "core/input.h"

// utility
#include "utility/stb.h"
#include "utility/mesh.h"
#include "utility/color.h"
#include "utility/assets.h"
#include "utility/texture.h"
#include "utility/camera.h"
#include "utility/transform.h"
#include "utility/scenegraph.h"
#include "utility/rendergraph.h"
#include "utility/filesystem.h"
#include "utility/material.h"
#include "utility/culling.h"
#include "utility/meshrenderer.h"
#include "utility/geometry.h"
#include "utility/rtbuilder.h"
#include "utility/arcball.h"
#include "utility/flycam.h"
#include "utility/time.h"
#include "utility/gltf.h"

// third party
#include <imgui.h>
#include <imgui_internal.h>
#include <imnodes.h>
#include <ImGuizmo.h>
#include <ImCurveEdit.h>
#include <ImSequencer.h>
#include <tiny_gltf.h>
#include "imgui/dearimgui.h"
#include "imgui/imgui_impl_vulkan.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

#endif // SLIM_HPP
