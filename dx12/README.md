## DirectX 12 version

![](images/dx12-1.png)

This is the DX12 version, the current version of Microsoft's graphics API. Compared to its predecessor it provides a lower-level abstraction over the GPU, similar to Vulkan. Draw calls and other operations are recorded in command lists submitted to dedicated GPU queues. Pipeline state objects aggregate most of the previously free state for rasterization, blending, depth testing, etc. Textures and buffers are passed to shaders through descriptors laid out by the application in GPU memory according to a root signature. Depending on how they are used, resources and rendertargets have to be transitioned using barriers. Frame synchronization using fences is also left to the application for maximum control.

![](images/dx12-2.png)

This version relies on a few simplifying assumptions: most resources and their descriptors are allocated at launch, resource barriers are handled manually, pipeline configuration is limited, descriptors and resource tables are allocated linearly and never released until the final cleanup. Mipmaps are generated manually using a compute shader as the API doesn't provide the corresponding helper function anymore.

![](images/dx12-3.png)


