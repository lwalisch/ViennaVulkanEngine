# The Vienna Vulkan Engine (VVE)

## About this fork

This is a port of the Vienna Vulkan Engine to other environments that don't use Visual Studio. I run this version on Ubuntu 18.04. It uses cmake and therefore can be used for developent with IDEs like CLion and Visual Studio Code.

## Changelog

- I had to remove Nuklear for this version of the project for now to keep it working

## TODO
- Make a documentation on how to get the project running on linux. (Short version: Check CMakeLists.txt for dependencies which should be available on the system, and install missing dependencies)

## Original Description

The Vienna Vulkan Engine (VVVE) is a Vulkan based render engine meant for learning and teaching the Vulkan API. It is open source under the MIT license. The VVE has been started as basis for game based courses at the Faculty of Computer Science of the University of Vienna, held by Prof. Helmut Hlavacs:

- https://ufind.univie.ac.at/de/course.html?lv=052214&semester=2018W
- https://ufind.univie.ac.at/de/course.html?lv=052212&semester=2018W
- https://ufind.univie.ac.at/de/course.html?lv=052211&semester=2019S

VVE's main contributor is Prof. Helmut Hlavacs (http://entertain.univie.ac.at/~hlavacs/). However, VVE will be heavily involved in the aforementioned courses, and other courses as well, and many students are already working on VVE extensions, porting, debugging etc.

VVE features are:
- 100% Vulkan, C++11, no fancy stuff
- Windowing through GLFW, other systems are possible.
- Multiplatform (almost) out of the box: Win 10, Linux, MacOS (using MoltenVK). See instructions below.
- Separation between the engine itself and an independent Vulkan helper layer below. The engine usually does not call Vulkan functions directly, but rather only helper functions, acting like macros. This reduces complexity.
- Simple callback usage through event listeners.
- Uses several OSS libraries and  - if-possible - single-header libraries for loading assets, multithreading, etc.
- Simple GUI based on the Nuklear library, other libraries like ImGUI are possible.
- Uses AMD's VMA Library for memory allocation.