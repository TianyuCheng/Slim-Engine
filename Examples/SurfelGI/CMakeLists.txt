compile_glsl(surfelgi_glsl_shaders shaders/)
add_executable(surfelgi
    main.cpp
    light.h
    config.h

    scene.h
    scene.cpp

    gbuffer.h
    gbuffer.cpp

    composer.h
    composer.cpp

    raytrace.h
    raytrace.cpp

    overlay.h
    overlay.cpp

    surfel.h
    surfel.cpp
)
target_link_libraries(surfelgi PRIVATE slim)
add_dependencies(surfelgi surfelgi_glsl_shaders)
