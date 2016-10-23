#Here be dragons

> Hic sunt dracones.

This repository contains multiple implementations of the same 3D scene, using different APIs and frameworks on various platforms. The goal is to provide a comparison between multiple rendering methods. This is inherently biased due to the variety of algorithms used and available CPU/GPU configurations, but can hopefully still provide interesting insights on 3D rendering.

![](images/image.png)

The following versions are currently available:

- [OpenGL](https://github.com/kosua20/GL_Template/tree/64652c408afc0d9fa4c9aa563cda09b9f585ecc8) 
- [WebGL](https://github.com/kosua20/herebedragons/tree/master/webgl)
- [SceneKit](https://github.com/kosua20/herebedragons/tree/master/scenekit)
- [Blender Cycles](https://github.com/kosua20/herebedragons/tree/master/cycles)
- [Nintendo DS](https://github.com/kosua20/herebedragons/tree/master/nds)

The following versions are planned:

- Metal
- Vulkan
- Unity
- PlayStation 2
- Ptah (a custom real-time renderer, see [the Github page](https://github.com/kosua20/PtahRenderer/))

In the directory of each version, one can find the source code, the corresponding ressources (as each method has its own format/quality constraints), along with a readme describing the method and a few examples. The only exception is the OpenGL project, which is described below.

##OpenGL