# glfw
# ---------------------------------------------------------------
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL  "GLFW lib only" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL  "GLFW lib only" FORCE)
set(GLFW_BUILD_INSTALL  OFF CACHE BOOL  "GLFW lib only" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL  "GLFW lib only" FORCE)
add_subdirectory(glfw)

# imgui
# ---------------------------------------------------------------
set(IMGUI_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends PARENT_SCOPE)
file(GLOB IMGUI_SOURCES  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp)
add_library(imgui SHARED ${IMGUI_SOURCES})
target_include_directories(imgui PRIVATE ${Vulkan_INCLUDE_DIRS})

# glm
# ---------------------------------------------------------------
add_subdirectory(glm)

# stb
# ---------------------------------------------------------------
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE stb/)

# tinyglf
# ---------------------------------------------------------------
set(TINYGLTF_BUILD_LOADER_EXAMPLE    OFF CACHE BOOL  "tinygltf lib only" FORCE)
set(TINYGLTF_BUILD_GL_EXAMPLES       OFF CACHE BOOL  "tinygltf lib only" FORCE)
set(TINYGLTF_BUILD_VALIDATOR_EXAMPLE OFF CACHE BOOL  "tinygltf lib only" FORCE)
set(TINYGLTF_BUILD_BUILDER_EXAMPLE   OFF CACHE BOOL  "tinygltf lib only" FORCE)
add_subdirectory(tinygltf)

# vulkan & vulkan memory allocator
# ---------------------------------------------------------------
find_package(Vulkan)
message(STATUS "Build with Vulkan Support: ${Vulkan_FOUND}")
if (${Vulkan_FOUND})
    # vulkan sdk
    message(STATUS "Vulkan Include Dirs: ${Vulkan_INCLUDE_DIRS}")
    message(STATUS "Vulkan Libraries: ${Vulkan_LIBRARIES}")

    # vulkan memory allocator
    set(VMA_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/src/)
    add_library(vma SHARED ${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/src/VmaUsage.cpp)
    set_target_properties(vma PROPERTIES CXX_STANDARD 14)
    target_include_directories(vma PRIVATE ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(vma PRIVATE ${Vulkan_LIBRARIES})

    # unified vulkan support settings
    set(VULKAN_SUPPORT ON PARENT_SCOPE)
    set(VULKAN_INCLUDES ${Vulkan_INCLUDE_DIRS} ${VMA_INCLUDE_DIRS} PARENT_SCOPE)
    set(VULKAN_LIBS ${Vulkan_LIBRARIES} vma PARENT_SCOPE)
else()
    # unified vulkan support settings
    set(VULKAN_SUPPORT OFF PARENT_SCOPE)
    set(VULKAN_INCLUDES "" PARENT_SCOPE)
    set(VULKAN_LIBS "" PARENT_SCOPE)
endif()
