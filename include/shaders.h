#pragma once
#include <glad/glad.h>
#include <cglm/mat4.h>
#include <cglm/vec4.h>

void prwsSetProjectionMatrix(mat4 matrix);

void prwsSetModelViewMatrix(mat4 matrix);

void prwsSetColorv(vec4 color);

vec4* prwsGetProjectionMatrix();

vec4* prwsGetModelViewMatrix();

float* prwsGetColor();

void prwsSetColor(float r, float g, float b, float a);

void prws_POS_shader();

void prws_POS_UV_shader();

void prws_POS_COLOR_shader();

void prws_POS_UV_COLOR_TEXID_shader();
