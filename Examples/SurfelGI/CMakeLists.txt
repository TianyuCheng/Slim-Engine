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
        shaders/surfelallocvis.frag
        shaders/surfelgridvis.frag

        shaders/gbuffer.frag
        shaders/gbuffer.vert

        shaders/raytrace.rgen
        shaders/shadow.rchit
        shaders/shadow.rmiss
        shaders/surfel.rchit
        shaders/surfel.rmiss
        shaders/surfel.rint

        shaders/surfelprepare.comp          # prepare for indirect call
        shaders/surfelgridreset.comp        # reset grid
        shaders/surfelupdate.comp           # update grid/cell count
        shaders/surfelgridoffset.comp       # recompute grid offset
        shaders/surfelbinning.comp          # fill grid cells
        shaders/surfelcov.comp              # coverage, allocate new surfels

    SPV
        vulkan1.2
)
