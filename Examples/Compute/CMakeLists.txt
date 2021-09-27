add_slim_project(
    TARGET  compute
    SOURCES main.cpp
    SHADERS shaders/simple.comp
    SPV     vulkan1.0
)
