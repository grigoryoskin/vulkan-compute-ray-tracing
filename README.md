# Vulkan compute shader based ray tracer.
<img width="600" alt="Screen Shot 2021-07-22 at 8 03 03" src="https://user-images.githubusercontent.com/44236259/126570824-43b49258-9bca-4f33-b756-573611cace79.png">

![ezgif-3-19e3095c79be](https://user-images.githubusercontent.com/44236259/126573565-160546f9-ba8d-41b6-917e-a1da974bb314.gif)

![ezgif-3-e0cf7c5acd74](https://user-images.githubusercontent.com/44236259/126500668-2999f992-3dda-4c17-84d6-3c1b116d000d.gif)

Ray tracer loosely based on [raytracing in one weekend series](https://raytracing.github.io), adapted for real time rendering on GPU.

## How it works.
Overall project structure comes from my [project template](https://github.com/grigoryoskin/vulkan-project-starter) with some changes to enable compute functionality.
Compute shader renders the ray traced scene into a texture that gets displayed onto a screen quad with a fragment shader.
[TextureOutputComputeModel](https://github.com/grigoryoskin/vulkan-compute-ray-tracing/blob/master/src/compute/TextureOutputComputeModel.h) holds the target texture, data buffers, pipeline and descriptor sets.

The [scene](https://github.com/grigoryoskin/vulkan-compute-ray-tracing/blob/master/src/compute/RtScene.h) consists of a an array of materials and an array of triangles. Each triangle holds a reference to the material. Reference is just material's index in the array for ease of use on GPU. 

[BVH](https://github.com/grigoryoskin/vulkan-compute-ray-tracing/blob/master/src/compute/Bvh.h) used to accelarate triangle search is a flat array too, since GPU doesn't support recursion.

Each frame [compute shader](https://github.com/grigoryoskin/vulkan-compute-ray-tracing/blob/master/resources/shaders/source/ray-trace-compute.comp) renders the scene with one sample per pixel and accumulates it in the taget texture.


## TODOs:
- [ ] Fix synchronization issues 😠 
- [ ] PBR materials.
- [ ] Monte Carlo integration.
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
