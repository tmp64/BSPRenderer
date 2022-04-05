#ifndef MATERIAL_SYSTEM_SHADER_TYPES_H
#define MATERIAL_SYSTEM_SHADER_TYPES_H

constexpr unsigned MAX_SHADER_TYPE_COUNT = 6;

//! A custom shader.
constexpr unsigned SHADER_TYPE_CUSTOM_IDX = 0;
constexpr unsigned SHADER_TYPE_CUSTOM = 1 << SHADER_TYPE_CUSTOM_IDX;

//! Blit shader.
//! Vertex inputs:
//!	  layout (location = 0) in vec3 aPos;
//!	  layout (location = 1) in vec2 aTexCoords;
constexpr unsigned SHADER_TYPE_BLIT_IDX = 1;
constexpr unsigned SHADER_TYPE_BLIT = 1 << SHADER_TYPE_BLIT_IDX;

//! Shader for ImGui, output in gamma space
constexpr unsigned SHADER_TYPE_IMGUI_IDX = 2;
constexpr unsigned SHADER_TYPE_IMGUI = 1 << SHADER_TYPE_IMGUI_IDX;

//! Shader for ImGui, output in linear space
constexpr unsigned SHADER_TYPE_IMGUI_LINEAR_IDX = 3;
constexpr unsigned SHADER_TYPE_IMGUI_LINEAR = 1 << SHADER_TYPE_IMGUI_LINEAR_IDX;

//! Shader for world brushes
constexpr unsigned SHADER_TYPE_WORLD_IDX = 4;
constexpr unsigned SHADER_TYPE_WORLD = 1 << SHADER_TYPE_WORLD_IDX;

//! Shader for brush entities
constexpr unsigned SHADER_TYPE_BRUSH_MODEL_IDX = 5;
constexpr unsigned SHADER_TYPE_BRUSH_MODEL = 1 << SHADER_TYPE_BRUSH_MODEL_IDX;

constexpr unsigned SHADER_TYPE_ALL = 0xFFFFFFFF;

constexpr const char *SHADER_TYPE_NAMES[] = {
    "SHADER_TYPE_CUSTOM",
    "SHADER_TYPE_BLIT",
    "SHADER_TYPE_IMGUI",
    "SHADER_TYPE_IMGUI_LINEAR",
    "SHADER_TYPE_WORLD",
    "SHADER_TYPE_BRUSH_MODEL",
};

static_assert(std::size(SHADER_TYPE_NAMES) == MAX_SHADER_TYPE_COUNT,
              "Forgot to update SHADER_TYPE_NAMES");

#endif
