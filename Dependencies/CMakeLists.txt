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

# vulkan & vulkan memory allocator
# ---------------------------------------------------------------
find_package(Vulkan)
message(STATUS "Build with Vulkan Support: ${Vulkan_FOUND}")
if(${Vulkan_FOUND})
  # vulkan sdk
  message(STATUS "Vulkan Include Dirs: ${Vulkan_INCLUDE_DIRS}")
  message(STATUS "Vulkan Libraries: ${Vulkan_LIBRARIES}")

  # vulkan memory allocator
  set(VMA_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/src/)
  add_library(
    vma STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/src/VmaUsage.cpp)
  set_target_properties(vma PROPERTIES CXX_STANDARD 14)
  target_include_directories(vma PRIVATE ${Vulkan_INCLUDE_DIRS})
  target_link_libraries(vma PRIVATE ${Vulkan_LIBRARIES})

  # unified vulkan support settings
  set(VULKAN_SUPPORT
      ON
      PARENT_SCOPE)
  set(VULKAN_INCLUDES
      ${Vulkan_INCLUDE_DIRS} ${VMA_INCLUDE_DIRS}
      PARENT_SCOPE)
  set(VULKAN_LIBS
      ${Vulkan_LIBRARIES} vma
      PARENT_SCOPE)
else()
  # unified vulkan support settings
  set(VULKAN_SUPPORT
      OFF
      PARENT_SCOPE)
  set(VULKAN_INCLUDES
      ""
      PARENT_SCOPE)
  set(VULKAN_LIBS
      ""
      PARENT_SCOPE)
endif()

# code formatter
# ---------------------------------------------------------------
add_subdirectory(Format.cmake)

# pybind11
# ---------------------------------------------------------------
set(PYBIND11_INSTALL
    OFF
    CACHE BOOL "pybind11 no install" FORCE)
set(PYBIND11_TEST
    OFF
    CACHE BOOL "pybind11 no test" FORCE)
add_subdirectory(pybind11)
