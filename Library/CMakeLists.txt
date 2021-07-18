# header files
set(SLIM_HEADERS
    slim/slim.hpp
)

# core class modules
set(SLIM_CORE_SOURCES
    core/debug.h
    core/buffer.h
    core/buffer.cpp
    core/commands.h
    core/commands.cpp
    core/context.h
    core/context.cpp
    core/framebuffer.h
    core/framebuffer.cpp
    core/image.h
    core/image.cpp
    core/sampler.h
    core/sampler.cpp
    core/pipeline.h
    core/pipeline.cpp
    core/renderframe.h
    core/renderframe.cpp
    core/renderpass.h
    core/renderpass.cpp
    core/shader.h
    core/shader.cpp
    core/synchronization.h
    core/synchronization.cpp
    core/vkutils.h
    core/vkutils.cpp
    core/window.h
    core/window.cpp
)

# vulkan-specific class modules
set(SLIM_IMGUI_SOURCES
    imgui/dearimgui.cpp
    imgui/dearimgui.h
    # personal modifications made to support imgui for custom texture
    imgui/imgui_impl_vulkan.h
    imgui/imgui_impl_vulkan.cpp
    ${PROJECT_SOURCE_DIR}/Dependencies/imgui/backends/imgui_impl_glfw.h
    ${PROJECT_SOURCE_DIR}/Dependencies/imgui/backends/imgui_impl_glfw.cpp
)

# utility class modules
set(SLIM_UTILITY_SOURCES
    utility/smartptr.h
    utility/transient.h
    utility/interface.h
    utility/stb.h
    utility/stb.cpp
    utility/gltf.h
    utility/gltf.cpp
    utility/color.h
    utility/color.cpp
    utility/texture.h
    utility/texture.cpp
    utility/rendergraph.h
    utility/rendergraph.cpp
    utility/scenegraph.h
    utility/scenegraph.cpp
    utility/transform.h
    utility/transform.cpp
    utility/camera.h
    utility/camera.cpp
    utility/mesh.h
    utility/mesh.cpp
    utility/skinnedmesh.h
    utility/skinnedmesh.cpp
    utility/material.h
    utility/material.cpp
    utility/animator.h
    utility/animator.cpp
    utility/assets.h
    utility/assets.cpp
)

add_library(slim SHARED
    ${SLIM_HEADERS}
    ${SLIM_CORE_SOURCES}
    ${SLIM_IMGUI_SOURCES}
    ${SLIM_UTILITY_SOURCES}
)
target_include_directories(slim PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IMGUI_INCLUDE_DIRS}
    ${IMNODES_INCLUDE_DIRS}
    ${VULKAN_INCLUDES}
)
target_link_libraries(slim PUBLIC glm glfw imgui imnodes stb tinygltf ghc_filesystem ${VULKAN_LIBS})
