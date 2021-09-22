compile_glsl(surfelgi_glsl_shaders shaders/)
add_executable(surfelgi
    main.cpp
    scene.h
    light.h

    gbuffer.h
    gbuffer.cpp

    composer.h
    composer.cpp

    raytrace.h
    raytrace.cpp

    overlay.h
    overlay.cpp
)
target_link_libraries(surfelgi PRIVATE slim)
add_dependencies(surfelgi surfelgi_glsl_shaders)
