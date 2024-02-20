## DirectX 11 version

![](images/dx11-1.png)

This version is an upgrade of the DX 9 version to use the DirectX 11 graphics API. Compared to its predecessor, additional GPU state is exposed to the developper and has to be explicitely set up. It is not possible anymore to send freeform parameters to shaders ; they are instead grouped in a constant buffer with specific alignment constraints, uploaded once per frame. Textures are exposed to the output merger and shaders through views that allow for reinterpretation of data. HLSL shaders are now targeting model 5 instead of 3, with differences in how textures and samplers are used. Vertex layout is now validated against the vertex shader input, prefiguring pipeline state compilation in more recent APIs.

As the workload is light, the app only use the immediate render context and doesn't rely on multi-threading with deferred contexts. Similarly, the default swapchain is created along with the device and context. Mipmap generation is performed automatically through the API.

![](images/dx11-2.png)

![](images/dx11-3.png)


