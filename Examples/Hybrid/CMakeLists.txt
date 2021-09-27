add_slim_project(
    TARGET hybrid

    SOURCES
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

    SHADERS
        shaders/compose.frag
        shaders/compose.vert
        shaders/gbuffer.frag
        shaders/gbuffer.vert
        shaders/shadow.rchit
        shaders/shadow.rgen
        shaders/shadow.rmiss

    SPV     vulkan1.2
)
