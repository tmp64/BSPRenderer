#ifndef RENDERER_ENVMAP_H
#define RENDERER_ENVMAP_H
#include <graphics/texture_cube.h>

//! Loads 6 BMP or TGA images into a cubemap.
TextureCube loadEnvMapImage(std::string_view textureName, std::string_view vpath);

//! @returns a checkerboard pattern cubemap.
TextureCube createErrorEnvMap(std::string_view textureName);

#endif
