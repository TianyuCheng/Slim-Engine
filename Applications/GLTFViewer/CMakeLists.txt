compile_glsl(gltfviewer_glsl_shaders shaders/)
add_executable(gltfviewer
    main.cpp
    model.cpp
    model.h
    material.cpp
    material.h)
target_link_libraries(gltfviewer PRIVATE slim)
add_dependencies(gltfviewer gltfviewer_glsl_shaders)
