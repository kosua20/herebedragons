## Blender version

![](images/cycles1.png)

Cyles is an offline rendering engine included in the Blender 3D editor. It performs path-tracing, simulating the light rays and their paths in the scene. This type of techniques is computationally expensive, and is mainly used in offline rendering. This simulation is physically accurate, and doesn't require any additional tricks for shadows rendering, camera focus, ambient occlusion, reflections,...

Parallax mapping is replaced by displacement mapping, where the plane is modeled as a dense grid of triangles, and the height map is applied directly on those vertices to displace them.

<blockquote class="twitter-video" data-lang="fr"><p lang="en" dir="ltr">And a rendered animation of the same Cycles scene (a bit jerky... some issue with the camera motion path). <a href="https://t.co/92Fa6jn1mY">pic.twitter.com/92Fa6jn1mY</a></p>&mdash; Simon Rodriguez (@simonkosua) <a href="https://twitter.com/simonkosua/status/788138982689366017">17 octobre 2016</a></blockquote> <script async src="//platform.twitter.com/widgets.js" charset="utf-8"></script>

![](images/cycles2.png)
*In the Blender editor.*

![](images/cycles3.png)


