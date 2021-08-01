cmake_minimum_required(VERSION 3.3.0)

#  ____            _           _
# |  _ \ _ __ ___ (_) ___  ___| |_
# | |_) | '__/ _ \| |/ _ \/ __| __|
# |  __/| | | (_) | |  __/ (__| |_
# |_|   |_|  \___// |\___|\___|\__|
#               |__/
# ---------------------------------------------------------------
project(slim LANGUAGES CXX)

#  ____                            _                 _
# |  _ \  ___ _ __   ___ _ __   __| | ___ _ __   ___(_) ___  ___
# | | | |/ _ \ '_ \ / _ \ '_ \ / _` |/ _ \ '_ \ / __| |/ _ \/ __|
# | |_| |  __/ |_) |  __/ | | | (_| |  __/ | | | (__| |  __/\__ \
# |____/ \___| .__/ \___|_| |_|\__,_|\___|_| |_|\___|_|\___||___/
#            |_|
# ---------------------------------------------------------------
add_subdirectory(Dependencies)
if (${VULKAN_SUPPORT})
    add_definitions(-DSLIM_VULKAN_SUPPORT)
endif()

#     _                 _
#    / \   ___ ___  ___| |_ ___
#   / _ \ / __/ __|/ _ \ __/ __|
#  / ___ \\__ \__ \  __/ |_\__ \
# /_/   \_\___/___/\___|\__|___/
#
# add assets directory
# ----------------------------------------------------------------
add_definitions(-DSLIM_ASSETS_DIRECTORY="${PROJECT_SOURCE_DIR}")

#   ____                      _ _
#  / ___|___  _ __ ___  _ __ (_) | ___ _ __
# | |   / _ \| '_ ` _ \| '_ \| | |/ _ \ '__|
# | |__| (_) | | | | | | |_) | | |  __/ |
#  \____\___/|_| |_| |_| .__/|_|_|\___|_|
#                      |_|
# ---------------------------------------------------------------
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
  add_compile_options(/W4 /WX-)
  add_compile_options("/std:c++latest")
else()
  set(CMAKE_CXX_FLAGS "-Wall -Wextra")
  set(CMAKE_CXX_FLAGS_DEBUG "-ggdb3")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

if (MSVC_VERSION GREATER_EQUAL "1900")
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
  if (_cpp_latest_flag_supported)
  endif()
endif()

# compilation database
# -----------------------------------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#  _     _ _                    _
# | |   (_) |__  _ __ __ _ _ __(_) ___  ___
# | |   | | '_ \| '__/ _` | '__| |/ _ \/ __|
# | |___| | |_) | | | (_| | |  | |  __/\__ \
# |_____|_|_.__/|_|  \__,_|_|  |_|\___||___/
# ---------------------------------------------------------------
add_subdirectory(Library)

#  ____  _               _
# / ___|| |__   __ _  __| | ___ _ __
# \___ \| '_ \ / _` |/ _` |/ _ \ '__|
#  ___) | | | | (_| | (_| |  __/ |
# |____/|_| |_|\__,_|\__,_|\___|_|
#
# ---------------------------------------------------------------
find_program(SHADERCOMPILER glslc)

function(compile_glsl Target Directory)
    file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${Directory}/*.vert"
        "${Directory}/*.frag"
        "${Directory}/*.comp"
    )

    foreach(GLSL ${GLSL_SOURCE_FILES})
        get_filename_component(FILENAME ${GLSL} NAME)
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${Directory})
        set(SPIRV "${CMAKE_CURRENT_BINARY_DIR}/${Directory}/${FILENAME}.spv")
        add_custom_command(
            OUTPUT  ${SPIRV}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${DIRECTORY}/"
            COMMAND ${SHADERCOMPILER} ${GLSL} -g -o ${SPIRV}
            DEPENDS ${GLSL}
            COMMENT "Building ${SPIRV}"
        )
        list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    endforeach(GLSL)

    add_custom_target(${Target} DEPENDS ${SPIRV_BINARY_FILES})
endfunction()

#  _____                           _
# | ____|_  ____ _ _ __ ___  _ __ | | ___  ___
# |  _| \ \/ / _` | '_ ` _ \| '_ \| |/ _ \/ __|
# | |___ >  < (_| | | | | | | |_) | |  __/\__ \
# |_____/_/\_\__,_|_| |_| |_| .__/|_|\___||___/
#                           |_|
# ---------------------------------------------------------------
add_subdirectory(Examples/BufferCopy)
add_subdirectory(Examples/Compute)
add_subdirectory(Examples/DearImGui)
add_subdirectory(Examples/DepthBuffering)
add_subdirectory(Examples/Multisampling)
add_subdirectory(Examples/TextureMapping)
add_subdirectory(Examples/RenderToTexture)
add_subdirectory(Examples/SceneGraph)
add_subdirectory(Examples/DynamicUniform)

#     _                _ _           _   _
#    / \   _ __  _ __ | (_) ___ __ _| |_(_) ___  _ __
#   / _ \ | '_ \| '_ \| | |/ __/ _` | __| |/ _ \| '_ \
#  / ___ \| |_) | |_) | | | (_| (_| | |_| | (_) | | | |
# /_/   \_\ .__/| .__/|_|_|\___\__,_|\__|_|\___/|_| |_|
#         |_|   |_|

add_subdirectory(Applications/GLTFViewer)
