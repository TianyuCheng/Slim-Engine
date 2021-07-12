# header files
set(SLIM_HEADERS
    slim/slim.hpp
)

# core class modules
set(SLIM_CORE_SOURCES
    core/buffer.cpp
    core/buffer.h

    core/color.cpp
    core/color.h

    core/commands.cpp
    core/commands.h

    core/context.cpp
    core/context.h

    core/dearimgui.cpp
    core/dearimgui.h

    core/debug.h

    core/framebuffer.cpp
    core/framebuffer.h

    core/image.cpp
    core/image.h

    core/texture.cpp
    core/texture.h

    core/sampler.cpp
    core/sampler.h

    core/pipeline.h
    core/pipeline.cpp

    core/renderframe.cpp
    core/renderframe.h

    core/rendergraph.cpp
    core/rendergraph.h

    core/renderpass.cpp
    core/renderpass.h

    core/shader.cpp
    core/shader.h

    core/synchronization.cpp
    core/synchronization.h

    core/vkutils.cpp
    core/vkutils.h

    core/window.cpp
    core/window.h
)

# vulkan-specific class modules
set(SLIM_IMGUI_SOURCES
    # personal modifications made to support imgui for custom texture
    imgui/imgui_impl_vulkan.h
    imgui/imgui_impl_vulkan.cpp
    ${PROJECT_SOURCE_DIR}/Dependencies/imgui/backends/imgui_impl_glfw.h
    ${PROJECT_SOURCE_DIR}/Dependencies/imgui/backends/imgui_impl_glfw.cpp
)

# utility class modules
set(SLIM_UTILITY_SOURCES
    utility/stb.h
    utility/stb.cpp
    utility/smartptr.h
    utility/transient.h
    utility/interface.h
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
target_link_libraries(slim PUBLIC glm glfw imgui imnodes stb tinygltf ${VULKAN_LIBS})
