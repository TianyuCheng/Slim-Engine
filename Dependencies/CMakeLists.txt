# code formatter
# ---------------------------------------------------------------
add_subdirectory(Format.cmake)

# glfw
# ---------------------------------------------------------------
set(GLFW_BUILD_EXAMPLES
    OFF
    CACHE BOOL "GLFW lib only" FORCE)
set(GLFW_BUILD_TESTS
    OFF
    CACHE BOOL "GLFW lib only" FORCE)
set(GLFW_BUILD_INSTALL
    OFF
    CACHE BOOL "GLFW lib only" FORCE)
set(GLFW_BUILD_DOCS
    OFF
    CACHE BOOL "GLFW lib only" FORCE)
add_subdirectory(glfw)

# imgui
# ---------------------------------------------------------------
set(IMGUI_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/imgui/
                       ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends)
set(IMGUI_INCLUDE_DIRS
    ${IMGUI_INCLUDE_DIRS}
    PARENT_SCOPE)
file(GLOB IMGUI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_INCLUDE_DIRS})

# imnodes
# ---------------------------------------------------------------
set(IMNODES_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/imnodes
    PARENT_SCOPE)
file(GLOB IMNODES_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/imnodes/*.cpp)
add_library(imnodes STATIC ${IMNODES_SOURCES})
target_link_libraries(imnodes imgui)
target_include_directories(imnodes PUBLIC ${IMNODES_INCLUDE_DIRS})

# imguizmo
# ---------------------------------------------------------------
set(IMGUIZMO_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo
    PARENT_SCOPE)
file(GLOB IMGUIZMO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/*.cpp)
add_library(imguizmo STATIC ${IMGUIZMO_SOURCES})
set_target_properties(imguizmo PROPERTIES CXX_STANDARD 17)
target_link_libraries(imguizmo imgui)
target_include_directories(imguizmo PUBLIC ${IMGUIZMO_INCLUDE_DIRS})

# glm
# ---------------------------------------------------------------
add_subdirectory(glm)

# stb
# ---------------------------------------------------------------
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE stb/)

# tinyglf
# ---------------------------------------------------------------
set(TINYGLTF_BUILD_LOADER_EXAMPLE
    OFF
    CACHE BOOL "tinygltf lib only" FORCE)
set(TINYGLTF_BUILD_GL_EXAMPLES
    OFF
    CACHE BOOL "tinygltf lib only" FORCE)
set(TINYGLTF_BUILD_VALIDATOR_EXAMPLE
    OFF
    CACHE BOOL "tinygltf lib only" FORCE)
set(TINYGLTF_BUILD_BUILDER_EXAMPLE
    OFF
    CACHE BOOL "tinygltf lib only" FORCE)
set(TINYGLTF_HEADER_ONLY
    ON
    CACHE INTERNAL "tinygltf lib only" FORCE)
set(TINYGLTF_INSTALL
    OFF
    CACHE INTERNAL "tinygltf lib only" FORCE)
add_subdirectory(tinygltf)

# add replacement for filesystem (header-only)
# ----------------------------------------------------------------
set(GHC_FILESYSTEM_BUILD_TESTING
    OFF
    CACHE BOOL "disable building tests for GHC" FORCE)
set(GHC_FILESYSTEM_BUILD_EXAMPLES
    OFF
    CACHE BOOL "disable building examples for GHC" FORCE)
set(GHC_FILESYSTEM_WITH_INSTALL
    OFF
    CACHE BOOL "disable install for GHC" FORCE)
add_subdirectory(filesystem EXCLUDE_FROM_ALL)

# volk metaloader
# ---------------------------------------------------------------
set(VOLK_PULL_IN_VULKAN
    ON
    CACHE BOOL "pull in vulkan library" FORCE)
set(VOLK_INSTALL
    OFF
    CACHE BOOL "disable volk install" FORCE)
add_subdirectory(volk)

# vulkan memory allocator
# ---------------------------------------------------------------
set(VMA_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/src/
    PARENT_SCOPE)
add_library(vma STATIC ${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/src/VmaUsage.cpp)
set_target_properties(vma PROPERTIES CXX_STANDARD 14)
target_link_libraries(vma PRIVATE volk volk_headers)
