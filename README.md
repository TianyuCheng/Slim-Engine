Slim Engine
===========
**Slim Engine** is a vulkan-based rendering engine for educational purpose.

Architecture
------------

* Frame Graph
    - Reference: [FrameGraph: Extensible Rendering Architecture in Frostbite](https://www.gdcvault.com/play/1024612/FrameGraph-Extensible-Rendering-Architecture-in)
    - **Slim Engine** implements a simplified FrameGraph. It benefits from the main concept of FrameGraph, and implements a functional, code driven framework for organizing resources and render pass dependencies within a frame.
    - Things I have implemented
        - virtual resources and passes creation
        - actual resource allocation and deallocation based on reference counting
        - automatic resource layout transition (attachments are transitioned by render passes already by Vulkan, textures layouts are transitioned by RenderGraph)
    - Things I have not implemnted
        - async compute. This is a must-have feature if we are going for complete GPU driven pipeline.
        - interface for preserve attachments. I have not really used preserve attachments in the existing examples. I will have them implemented when I need it.
        - interface for multi-subpass render passes (useful for tile-based architecture). This feature is differently exposed across different APIs (DX12, Metal). I need to think about how to expose it.

* Scene Graph
    - **Slim Engine** implements a primitive scene graph system.
    - Each scene node contains a **Transform** object, which provides functionality for local transformation.
    - Each scene node contains a **Submesh** and a **Material** for rendering.
    - **Render()** will filter out the culled objects, and sort objects based on object types and distance.
        - Opaque objects are sorted from front to back.
        - Background objects (e.g. Skybox) are sorted from front to back.
        - Unordered objects (e.g. OIT rendered particles) are not sorted.
        - Transparent objects are sorted from back to front for correct blending.
    - **Render()** will group objects by material to avoid frequent descriptor update.

* Material System
    - **Slim Engine** implements a low level material system, where material is mostly just a wrapper for pipeline object.
    - Each material contains exactly one graphics pipeline. It is responsible for binding graphics pipeline, binding material-specific descriptors.
    - Material must provide 2 function objects: **PrepareMaterial** and **PrepareSceneNode** for updating descriptors for material and scene nodes respectively.

Examples
--------
During development, I have written progressively written some simple examples
demonstrating how to use this library.

Examples are located in `Examples/` directory. Here's a list of the examples.

* Buffer Copy
    - This is an example for data copying between buffers.

* Compute
    - This is an example launching a simple compute kernel, with input/output buffers.

* Dear ImGui
    - This is an example showcasing integration of a well-known immediate mode GUI library (Dear ImGui).

* Depth Buffering
    - This is an example showcasing how to enable depth testing with with depth/stencil attachment.

* Multisampling
    - This is an example showcasing how to enable multisampling with resolve attachment.

* Texture Mapping
    - This is an example showcasing how to load and setup 2D texture for rendering.

* Render to Texture
    - This is an example showcasing how to do render to texture with multipass frame graph.

* Scene Graph
    - This is an example showcasing how to use hierarchical scene graph for scene management.

Applications
------------

* Post-Processing
    * [TODO] Bloom Effect
    * [TODO] Screen Space Ambient Occlusion (SSAO)
    * [TODO] Screen Space Subsurface Scattering (SSSS)

* HiZ Occlusion Culling
    - [TODO]

* Deferred Rendering
    - [TODO]

* Shadow Mapping
    - [TODO]

* Skeletal Animation
    - [TODO]

* Temporal Anti-Aliasing (TAA)
    - [TODO]

* Particle System / Order Indepdenent Transparency (OIT)
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

* stduuid
    - a library for uuid generation

* imgui (DearImGui)
	- immediate mode GUI library

* imnodes
	- a node-graph UI based on imgui

Author(s)
---------
Tianyu Cheng

[tianyu.cheng@uteaxs.edu](mailto:tianyu.cheng@uteaxs.edu)
