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

# compilation database
# -----------------------------------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#     _                 _
#    / \   ___ ___  ___| |_ ___
#   / _ \ / __/ __|/ _ \ __/ __|
#  / ___ \\__ \__ \  __/ |_\__ \
# /_/   \_\___/___/\___|\__|___/
#
# add assets directory
# ----------------------------------------------------------------
add_definitions(-DSLIM_LIB_ASSETS_DIRECTORY="${PROJECT_SOURCE_DIR}/Library/assets")
add_definitions(-DSLIM_USR_ASSETS_DIRECTORY="${PROJECT_SOURCE_DIR}/Assets")

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
    add_compile_options(/W1 /WX-)
    add_compile_options("/std:c++latest")
    add_definitions(-D_USE_MATH_DEFINES)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS YES CACHE BOOL "Export all symbols")
else()
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-ggdb3")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        # do something
    endif()
endif()

#  ____  _               _
# / ___|| |__   __ _  __| | ___ _ __
# \___ \| '_ \ / _` |/ _` |/ _ \ '__|
#  ___) | | | | (_| | (_| |  __/ |
# |____/|_| |_|\__,_|\__,_|\___|_|
#
# ---------------------------------------------------------------
find_program(CMAKE_GLSL_COMPILER glslc)
mark_as_advanced(CMAKE_GLSL_COMPILER)

function(add_slim_project)
    # parse args
    set(prefix SLIM_PROJECT)
    set(flags "")
    set(singleValues TARGET SPV)
    set(multiValues SOURCES SHADERS)
    include(CMakeParseArguments)
    cmake_parse_arguments(${prefix}
                     "${flags}"
                     "${singleValues}"
                     "${multiValues}"
                      ${ARGN})

    # generate shader objects
    foreach(SHADER ${SLIM_PROJECT_SHADERS})
        get_filename_component(FILENAME  ${SHADER} NAME)
        get_filename_component(DIRECTORY ${SHADER} DIRECTORY)
        file(MAKE_DIRECTORY ${DIRECTORY})
        set(INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}/${FILENAME}")
        set(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${DIRECTORY}/${FILENAME}.spv")
        set(DEPFILE "${CMAKE_CURRENT_BINARY_DIR}/${DIRECTORY}/${FILENAME}.spv.d")
        set(INCLUDE "${PROJECT_SOURCE_DIR}/Library/shaderlib")
        add_custom_command(
            OUTPUT  ${OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${DIRECTORY}/"
            COMMAND ${SHADERCOMPILER} -MD --target-env=${SLIM_PROJECT_SPV} -I ${INCLUDE} ${INPUT} -g -O -o ${OUTPUT}
            DEPENDS ${INPUT}
            # DEPFILE ${DEPFILE} # This is not supported by Visual Studio 2019
            COMMENT "Building shader: ${OUTPUT}"
        )
        list(APPEND SPIRV_BINARY_FILES ${OUTPUT})
    endforeach(SHADER)

    # generate project target with both sources and shaders
    add_executable(${SLIM_PROJECT_TARGET}
                   ${SLIM_PROJECT_SOURCES}
                   ${SPIRV_BINARY_FILES})
    target_link_libraries(${SLIM_PROJECT_TARGET} PRIVATE slim)
endfunction()

#  _     _ _                    _
# | |   (_) |__  _ __ __ _ _ __(_) ___  ___
# | |   | | '_ \| '__/ _` | '__| |/ _ \/ __|
# | |___| | |_) | | | (_| | |  | |  __/\__ \
# |_____|_|_.__/|_|  \__,_|_|  |_|\___||___/
# ---------------------------------------------------------------
add_subdirectory(Library)

#  _____                           _
# | ____|_  ____ _ _ __ ___  _ __ | | ___  ___
# |  _| \ \/ / _` | '_ ` _ \| '_ \| |/ _ \/ __|
# | |___ >  < (_| | | | | | | |_) | |  __/\__ \
# |_____/_/\_\__,_|_| |_| |_| .__/|_|\___||___/
#                           |_|
# ---------------------------------------------------------------
add_subdirectory(Examples)

#     _                _ _           _   _
#    / \   _ __  _ __ | (_) ___ __ _| |_(_) ___  _ __
#   / _ \ | '_ \| '_ \| | |/ __/ _` | __| |/ _ \| '_ \
#  / ___ \| |_) | |_) | | | (_| (_| | |_| | (_) | | | |
# /_/   \_\ .__/| .__/|_|_|\___\__,_|\__|_|\___/|_| |_|
#         |_|   |_|
# ---------------------------------------------------------------
add_subdirectory(Applications)

#  _____         _
# |_   _|__  ___| |_ ___
#   | |/ _ \/ __| __/ __|
#   | |  __/\__ \ |_\__ \
#   |_|\___||___/\__|___/
# ---------------------------------------------------------------
add_subdirectory(Tests)
