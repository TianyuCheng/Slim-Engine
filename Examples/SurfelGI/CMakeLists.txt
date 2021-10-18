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

        overlay.h
        overlay.cpp

        render.h
        render.cpp

    SHADERS
        shaders/gbuffer/gbuffer.vert
        shaders/gbuffer/gbuffer.frag

        shaders/debug/depthvis.comp
        shaders/debug/objectvis.comp
        shaders/debug/gridvis.comp
        shaders/debug/allocvis.comp

        shaders/surfel/reset.comp
        shaders/surfel/offset.comp
        shaders/surfel/prepare.comp
        shaders/surfel/update.comp
        shaders/surfel/binning.comp
        shaders/surfel/coverage.comp

        # shaders/raytrace/raytrace.rgen
        # shaders/raytrace/raytrace.rchit
        # shaders/raytrace/raytrace.rchit

    SPV
        vulkan1.2
)
