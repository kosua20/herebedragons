# Here be dragons

> Hic sunt dracones.

This repository contains multiple implementations of the same 3D scene, using different APIs and frameworks on various platforms. The goal is to provide a comparison between multiple rendering methods. This is inherently biased due to the variety of algorithms used and available CPU/GPU configurations, but can hopefully still provide interesting insights on 3D rendering. You can check a video of all current versions runnning [here (Youtube)](https://www.youtube.com/watch?v=Bbpu34S2bSI)

![](image.png)

The following versions are currently available:

- [OpenGL](https://github.com/kosua20/herebedragons/tree/master/opengl)
- [WebGL](https://github.com/kosua20/herebedragons/tree/master/webgl) 
- [Vulkan](https://github.com/kosua20/herebedragons/tree/master/vulkan) 
- [Metal](https://github.com/kosua20/herebedragons/tree/master/metal)
- [SceneKit](https://github.com/kosua20/herebedragons/tree/master/scenekit)
- [Unity](https://github.com/kosua20/herebedragons/tree/master/unity)
- [Blender Cycles](https://github.com/kosua20/herebedragons/tree/master/cycles)
- [Ptah](https://github.com/kosua20/herebedragons/tree/master/ptah) (a custom real-time renderer, see [the Github page](https://github.com/kosua20/PtahRenderer/))
- [Nintendo DS](https://github.com/kosua20/herebedragons/tree/master/nds)
- [Nintendo Game Boy Advance](https://github.com/kosua20/herebedragons/tree/master/gba)
- [PlayStation 2](https://github.com/kosua20/herebedragons/tree/master/ps2)  
- [PICO-8](https://github.com/kosua20/herebedragons/tree/master/pico8) 
- [Vectrex](https://github.com/kosua20/herebedragons/tree/master/vectrex) 
- [Glide](https://github.com/kosua20/herebedragons/tree/master/glide)

The following versions are (tentatively) planned:

- DirectX (11 or 12), OpenGL ES
- other console hardwares
- another raytracing approach

In the directory of each version, one can find the source code, the corresponding resources (as each method has its own format/quality constraints), along with a readme describing the method and a few examples. 

## Resources
The scene is composed of four main elements:

- a plane representing the ground, with a pavement texture.
- a dragon (the Stanford dragon).
- a monkey head ('Suzanne') rotating around its vertical axis.
- a skybox surrounding the scene, textured with a cloudy sky.

Additionally, the sun is simulated by using a directional light, and the camera should be able to rotate around the scene.
The initial 3D models and textures are contained in the `opengl/resources` [directory](https://github.com/kosua20/herebedragons/tree/master/opengl/resources).

## Other implementations
Here you can find implementations using some of the above APIs combined with other programming languages, submitted by awesome people!

- An OpenGL + Python version, by cprogrammer1994: [herebedragons-python](https://github.com/cprogrammer1994/herebedragons-python). (see [#11](https://github.com/kosua20/herebedragons/issues/11))
- A Vulkan version, by vazgriz: [vk_dragons](https://github.com/vazgriz/vk_dragons).
