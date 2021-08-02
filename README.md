# Vulkan compute shader based ray tracer.

<img width="500" alt="Screen Shot 2021-08-01 at 18 36 16" src="https://user-images.githubusercontent.com/44236259/127766493-e2402bde-48ca-462a-8110-d849151e9d18.png">

![ezgif-7-7a97c6e30f17](https://user-images.githubusercontent.com/44236259/127881165-f86d19b0-65f0-4b07-81e6-2ff1b92eea1e.gif)

Ray tracer loosely based on [raytracing in one weekend series](https://raytracing.github.io), adapted for real time rendering on GPU.

## How it works.
Overall project structure comes from my [project template](https://github.com/grigoryoskin/vulkan-project-starter) with some changes to enable compute functionality.
Compute shader renders the ray traced scene into a texture that gets displayed onto a screen quad with a fragment shader.
[TextureOutputComputeModel](https://github.com/grigoryoskin/vulkan-compute-ray-tracing/blob/master/src/compute/TextureOutputComputeModel.h) holds the target texture, data buffers, pipeline and descriptor sets.

The [scene](https://github.com/grigoryoskin/vulkan-compute-ray-tracing/blob/master/src/compute/RtScene.h) consists of a an array of materials and an array of triangles. Each triangle holds a reference to the material. Reference is just material's index in the array for ease of use on GPU. 

[BVH](https://github.com/grigoryoskin/vulkan-compute-ray-tracing/blob/master/src/compute/Bvh.h) used to accelarate triangle search is a flat array too, since GPU doesn't support recursion.

Each frame [compute shader](https://github.com/grigoryoskin/vulkan-compute-ray-tracing/blob/master/resources/shaders/source/ray-trace-compute.comp) renders the scene with one sample per pixel and accumulates it in the taget texture.

WIP

## TODOs:
- [ ] Fix synchronization issues 😠 
- [X] Glass materials.
- [ ] Fog.
- [ ] PBR materials.
- [X] Light sampling
- [ ] Try "roped" bvh to see how it improves performance.

## How to run
This is an instruction for mac os, but it should work for other systems too, since all the dependencies come from git submodules and build with cmake.
1. Download and install [Vulkan SDK] (https://vulkan.lunarg.com)
2. Pull glfw, glm, stb and obj loader:
```
git submudule init
git submodule update
```
3. Create a buld folder and step into it.
```
mkdir build
cd build
```
4. Run cmake. It will create `makefile` in build folder.
```
cmake -S ../ -B ./
```
5. Create an executable with makefile.
```
make
```
6. Compile shaders. You might want to run this with sudo if you dont have permissions for write.
```
mkdir ../resources/shaders/generated
sh ../compile.sh
```
7. Run the executable.
```
./vulkan
```
