BSPRenderer
===========
This is a Half-Life 1 BSP loader and renderer written in C++ and OpenGL 3.3 Core.

Build requirements
------------
- OpenGL mathematics library (GLM)
- SDL2 library

**Note**: code is designed to be portable but was only tested on Windows with Visual Studio 2017. Some patches may be required.


Lighting
--------
The renderer suppports both GoldSrc lightmaps (in the .bsp file) and custom ones
generated using `rad` utility.

Screenshots
-----------
**All these screenshots are outdated!**

![Crossfire with basic lighting](/_screenshots/bspviewer_crossfire.png?raw=true "Crossfire with basic lighting")

![Preview of custom lighting](/_screenshots/custom_lighting.png?raw=true "Preview of custom lighting")

![Preview of custom lighting (lightmap only)](/_screenshots/custom_lighting_lightmap.png?raw=true "Preview of custom lighting  (lightmap only)")

Credits
-------
A lot of rendering code is based on Xash3D so thanks to everyone who worked on it. Also thanks to id Software for open-sourcing Quake engine code which helped massively.
