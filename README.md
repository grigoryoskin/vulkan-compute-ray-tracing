# Vulkan compute to texture



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
sh ../compile.sh
```
7. Run the executable.
```
./vulkan
```
