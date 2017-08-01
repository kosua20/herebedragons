# Contributing

First of all, thank you for your interest in this project! It started as an educational exercise for myself, to get a feel of various existing rendering APIs and engines; but can also be useful to other people (I hope :-))

## New versions

If you want to add a new version, I would suggest first opening an issue with the **'additional version'** tag: somebody might already be working on it. 
Please keep in mind that each new version should implement the same scene using either: 

- a high-level rendering engine (*Unreal Engine*, *CryEngine*, ...)
- a lower-level toolset (for instance *Godot*, *ThreeJS*, ...) 
- or an even-lower-level API (*OpenGLES*, *DirectX*, ...). 

Homebrew versions and support for old/weird platforms or APIs are more than welcomed!

If you would rather want to rewrite one of the existing versions in another programming language (Go + OpenGL or Objective-C + Metal, for instance), I would rather suggest that you create a new fork, but feel free to add a link to it in the main `README.md`.

## Submitting

When everything is ready, submit a pull request with your new version placed in a directory at the root of the repository (the same way as the existing versions). Please add a `README.md` file with some details on the toolchain used, a few images of the result, any precisions you think are necessary, and feel free to credit yourself :) Also add a link in the root `README.md`.

## Models

Feel free to use the models and textures, their highest quality version is in the [ressources directory]((https://github.com/kosua20/GL_Template/tree/3de4e116cdd24df300fda42326a7a4e431f7f861/ressources)) of the OpenGL version. The 3D models of Suzanne and the Stanford dragon remain under their original licenses, same for the Miramar cubemap (credit of Jockum Skoglund/Hipshot).