Slim Engine
===========
**Slim Engine** is a vulkan-based rendering engine for educational purpose.

Architecture
------------

* Frame Graph
    - [TODO]

* Scene Graph
    - [TODO]

Examples
--------
During development, I have written progressively written some simple examples
demonstrating how to use this library.


Examples are located in `Examples/` directory. Here's a list of the examples.

* Transfer
    - This is a demo for data copying between buffers.

* Compute
    - This is a demo for launching a simple compute kernel, with input/output buffers.

* Dear ImGui
    - This is a demo for showcasing integration of a well-known immediate mode GUI library (DearImGui).

* Depth Buffering
    - This is a demo showcasing how to enable depth testing with depth buffering.

* Texture Mapping
    - [TODO]

* Render to Texture
    - [TODO]

* Deferred Rendering
    - [TODO]

* Shadow Mapping
    - [TODO]

* Cube Marching
    - [TODO]

* Ray Marching
    - [TODO]

* Volumetric Light and Shadows
    - [TODO]

* Path Tracing
    - [TODO]

Dependencies
------------

* Vulkan
	- Vulkan backend API for low-level rendering

* VulkanMemoryAllocator
	- an efficient memory allocator for vulkan buffer/image allocation

* glm
	- glsl style math library

* stb
	- a collection of utility functions, including image read/write

* tinygltf
	- a library for gltf loading

* imgui (DearImGui)
	- immediate mode GUI library

* imnodes
	- a node-graph UI based on imgui

Author(s)
---------
Tianyu Cheng

[tianyu.cheng@uteaxs.edu](mailto:tianyu.cheng@uteaxs.edu)
