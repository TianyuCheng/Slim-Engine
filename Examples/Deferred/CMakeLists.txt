add_slim_project(
    TARGET
        deferred
    SOURCES
        main.cpp
        fairy.h
        scene.h
    SHADERS
        shaders/fairy.frag
        shaders/fairy.vert
        shaders/fairylight.frag
        shaders/fairylight.vert
        shaders/fairymask.frag
        shaders/fairymask.vert
        shaders/gbuffer.frag
        shaders/gbuffer.vert
    SPV
        vulkan1.0
)
