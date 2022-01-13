#ifndef MATERIAL_SYSTEM_SHADER_TYPES_H
#define MATERIAL_SYSTEM_SHADER_TYPES_H

constexpr unsigned MAX_SHADER_TYPE_COUNT = 3;

//! Blit shader.
//! Vertex inputs:
//!		TODO
constexpr unsigned SHADER_TYPE_BLIT_IDX = 0;
constexpr unsigned SHADER_TYPE_BLIT = 1 << SHADER_TYPE_BLIT_IDX;

//! Shader for ImGui, output in gamma space
constexpr unsigned SHADER_TYPE_IMGUI_IDX = 1;
constexpr unsigned SHADER_TYPE_IMGUI = 1 << SHADER_TYPE_IMGUI_IDX;

//! Shader for ImGui, output in linear space
constexpr unsigned SHADER_TYPE_IMGUI_LINEAR_IDX = 2;
constexpr unsigned SHADER_TYPE_IMGUI_LINEAR = 1 << SHADER_TYPE_IMGUI_LINEAR_IDX;

constexpr unsigned SHADER_TYPE_ALL = 0xFFFFFFFF;

constexpr const char *SHADER_TYPE_NAMES[MAX_SHADER_TYPE_COUNT] = {
    "SHADER_TYPE_BLIT",
    "SHADER_TYPE_IMGUI",
    "SHADER_TYPE_IMGUI_LINEAR",
};

#endif
