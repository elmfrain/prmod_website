#pragma once
#include <glad/glad.h>
#include <cglm/mat4.h>
#include <cglm/vec4.h>

void prwsSetProjectionMatrix(mat4 matrix);

void prwsSetModelViewMatrix(mat4 matrix);

void prwsSetColorv(vec4 color);

float* prwsGetProjectionMatrix();

float* prwsGetModelViewMatrix();

float* prwsGetColor();

void prwsSetColor(float r, float g, float b, float a);

void prwsUsePOSshader();

void prwsUsePOS_UVshader();