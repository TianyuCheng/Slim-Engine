compile_glsl(gltfviewer_glsl_shaders shaders/)
add_executable(gltfviewer
    main.cpp
    viewer.cpp
    viewer.h)
target_link_libraries(gltfviewer PRIVATE slim)
add_dependencies(gltfviewer gltfviewer_glsl_shaders)
