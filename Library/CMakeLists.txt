# header files
file(GLOB SLIM_HEADERS slim *.hpp)

# core class modules
file(GLOB SLIM_CORE_HEADERS core/*.h)
file(GLOB SLIM_CORE_SOURCES core/*.cpp)

# utility class modules
file(GLOB SLIM_UTIL_HEADERS utility/*.h)
file(GLOB SLIM_UTIL_SOURCES utility/*.cpp)

# imgui class modules
file(GLOB SLIM_IMGUI_HEADERS imgui/*.h)
file(GLOB SLIM_IMGUI_SOURCES imgui/*.cpp)

add_library(slim STATIC
    ${SLIM_HEADERS}
    ${SLIM_CORE_HEADERS}
    ${SLIM_CORE_SOURCES}
    ${SLIM_UTIL_HEADERS}
    ${SLIM_UTIL_SOURCES}
    ${SLIM_IMGUI_SOURCES}
    ${PROJECT_SOURCE_DIR}/Dependencies/imgui/backends/imgui_impl_glfw.h
    ${PROJECT_SOURCE_DIR}/Dependencies/imgui/backends/imgui_impl_glfw.cpp
)
set_target_properties(slim PROPERTIES CXX_STANDARD 17)
target_include_directories(slim PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IMGUI_INCLUDE_DIRS}
    ${IMNODES_INCLUDE_DIRS}
    ${IMGUIZMO_INCLUDE_DIRS}
    ${VULKAN_INCLUDES}
)
target_link_libraries(slim PUBLIC glm glfw imgui imnodes imguizmo stb tinygltf ghc_filesystem ${VULKAN_LIBS})
