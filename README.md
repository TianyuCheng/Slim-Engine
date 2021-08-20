Slim Engine
===========
**Slim Engine** is a vulkan-based rendering engine for educational purpose.

Architecture
------------

* Render Graph
    - Reference: [FrameGraph: Extensible Rendering Architecture in Frostbite](https://www.gdcvault.com/play/1024612/FrameGraph-Extensible-Rendering-Architecture-in)
    - **Slim Engine** implements a simplified FrameGraph. It benefits from the main concept of FrameGraph, and implements a functional, code driven framework for organizing resources and render pass dependencies within a frame.
    - Things I have implemented
        - virtual resources and passes creation
        - actual resource allocation and deallocation based on reference counting
        - automatic resource layout transition (attachments are transitioned by render passes already by Vulkan, textures layouts are transitioned by RenderGraph)
        - async compute, which is a must-have feature if we are going for complete GPU driven pipeline (however, I have not validated the correctness of this feature)
    - Things I have not implemnted
        - interface for preserve attachments. I have not really used preserve attachments in the existing examples. I will have them implemented when I need it.
        - interface for multi-subpass render passes (useful for tile-based architecture). This feature is differently exposed across different APIs (DX12, Metal). I need to think about how to expose it.

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
	- ![Screenshot](./Examples/DearImGui/screenshot.png =100x)

* Depth Buffering
    - This is an example showcasing how to enable depth testing with with depth/stencil attachment.
	- ![Screenshot](./Examples/DepthBuffering/screenshot.png =100x)

* Multisampling
    - This is an example showcasing how to enable multisampling with resolve attachment.
	- ![Screenshot](./Examples/Multisampling/screenshot.png =100x)

* Texture Mapping
    - This is an example showcasing how to load and setup 2D texture for rendering.
	- ![Screenshot](./Examples/TextureMapping/screenshot.png =100x)

* Render to Texture
    - This is an example showcasing how to do render to texture with multipass frame graph.
	- ![Screenshot](./Examples/RenderToTexture/screenshot.png =100x)

* Scene Graph
    - This is an example showcasing how to use hierarchical scene graph for scene management.
	- ![Screenshot](./Examples/SceneGraph/screenshot.png =100x)

* Geometries
    - This is an example showcasing how to use slim's geometry data generation API.
	- ![Screenshot](./Examples/Geometries/screenshot.png =100x)

* Descriptor Indexing
    - This is an example showcasing how to use descriptor indexing feature provided by vulkan.
    - This is a feature used heavily for GPU-driven pipeline.
	- ![Screenshot](./Examples/DescriptorIndexing/screenshot.png =100x)

* Skybox
    - This is an example showcasing how to setup a skybox, and implement reflect/refract materials.
	- ![Screenshot](./Examples/Geometries/screenshot.png =100x)

Applications
------------

* GLTFViewer
    - Implement a basic gltfviewer, with physically-based rendering (PBR) shaders.

Dependencies
------------

* Vulkan
* VulkanMemoryAllocator
* glm
* stb
* tinygltf
* imgui (DearImGui)
* imnodes

Author(s)
---------
Tianyu Cheng

[tianyu.cheng@uteaxs.edu](mailto:tianyu.cheng@uteaxs.edu)
