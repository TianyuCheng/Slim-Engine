add_slim_project(
    TARGET
        surfelgi

    SOURCES
        main.cpp
        config.h

        scene.h
        scene.cpp

        update.h
        update.cpp

        debug.h
        debug.cpp

        gbuffer.h
        gbuffer.cpp

        surfel.h
        surfel.cpp

        compose.h
        compose.cpp

        overlay.h
        overlay.cpp

        direct.h
        direct.cpp

        render.h
        render.cpp

    SHADERS
        shaders/gbuffer/gbuffer.vert
        shaders/gbuffer/gbuffer.frag

        shaders/debug/depthvis.comp
        shaders/debug/objectvis.comp
        shaders/debug/gridvis.comp
        shaders/debug/allocvis.comp
        shaders/debug/geometry.vert
        shaders/debug/light.frag

        shaders/surfel/reset.comp
        shaders/surfel/offset.comp
        shaders/surfel/prepare.comp
        shaders/surfel/update.comp
        shaders/surfel/binning.comp
        shaders/surfel/coverage.comp

        shaders/raytrace/ray.rgen
        shaders/raytrace/direct.rgen
        shaders/raytrace/scene.rchit
        shaders/raytrace/scene.rmiss
        shaders/raytrace/shadow.rmiss

        shaders/post/fullscreen.vert
        shaders/post/compose.frag

    SPV
        vulkan1.2
)
