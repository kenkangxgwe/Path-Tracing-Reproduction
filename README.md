# Path Tracing Reproduction
A path tracing implementation based on the OptiX SDK sample.

## Build
### Requirements
#### Windows 10
* **[CUDA Toolkit](https://developer.nvidia.com/cuda-downloads?)** 8.0 or above
* **[OptiX Ray Tracing Engine](https://developer.nvidia.com/optix)** 4.1.1
* **Visual Studio 2015** or **Visual Studio 2017** with [VS2015 MSVC v140 Toolset](https://blogs.msdn.microsoft.com/vcblog/2017/11/02/visual-studio-build-tools-now-include-the-vs2017-and-vs2015-msvc-toolsets/)

1. Use **CMake** to build the project. Select "Visual Studio 14 2015 Win64" or "Visual Studio 15 2017 Win64"  as the generator for the project.

2. In case you use Visual Studio 2017, select all projects, open the properties dialog and change the "Platform Toolset" to **Visual Studio 2015 (v140)**.

3. Build the solution.

4. Reload the Solution if prompt. Goto step 2 until all projects succeed.

## Screenshots
### Original
![cbox_ori](https://github.com/kenkangxgwe/Path-Tracing-Reproduction/blob/master/data/doc/original.png?raw=true)
### Filtered
![cbox_fil](https://github.com/kenkangxgwe/Path-Tracing-Reproduction/blob/master/data/doc/filtered.png?raw=true)
