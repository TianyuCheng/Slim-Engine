add_slim_project(
    TARGET
        surfelgi

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

        visualize.h
        visualize.cpp

        surfel.h
        surfel.cpp

    SHADERS
        shaders/fullscreen.vert
        shaders/compose.frag
        shaders/objectvis.frag
        shaders/lineardepth.frag

        shaders/gbuffer.frag
        shaders/gbuffer.vert

        shaders/shadow.rchit
        shaders/shadow.rgen
        shaders/shadow.rmiss

        shaders/surfelcov.comp

    SPV
        vulkan1.2
)
