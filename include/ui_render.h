#pragma once

#include <stdint.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cglm/mat4.h>
#include <cglm/vec4.h>

#define PRWUI_TO_LEFT 0
#define PRWUI_TO_RIGHT 1
#define PRWUI_TO_BOTTOM 2
#define PRWUI_TO_TOP 3

#define PRWUI_TOP_LEFT 4
#define PRWUI_TOP_CENTER 5
#define PRWUI_TOP_RIGHT 6
#define PRWUI_MID_LEFT 7
#define PRWUI_CENTER 8
#define PRWUI_MID_RIGHT 9
#define PRWUI_BOTTOM_LEFT 10
#define PRWUI_BOTTOM_CENTER 11
#define PRWUI_BOTTOM_RIGHT 12

void prwuiSetWindow(GLFWwindow* window);

void prwuiSetupUIrendering();

void prwuiGenQuad(float left, float top, float right, float bottom, uint32_t color, int texID);

void prwuiGenGradientQuad(int direction, float left, float top, float right, float bottom, uint32_t startColor, uint32_t endColor, int texID);

void prwuiGenVerticalLine(float x, float startY, float endY, uint32_t color);

void prwuiGenHorizontalLine(float y, float startX, float startY, uint32_t color);

void prwuiGenString(int anchor, const char* str, float x, float y, uint32_t color);

void prwuiGenIcon(const char* iconName, float x, float y, float scale);

void prwuiRenderBatch();

float prwuiGetStringHeight();

float prwuiGetStringWidth(const char* str);

float prwuiGetCharWidth(int unicode);

int prwfrSplitToFit(char* dst, const char* src, float maxWidth);

int prwfrSplitToFitr(char* dst, const char* src, float maxWidth, const char* regex);

void prwfrGetFormats(char* dst, const char* src);

int prwfrUnicodeFromUTF8(const unsigned char* str, int* bytesRead);

int prwuiGetWindowWidth();

int prwuiGetWindowHeight();

float prwuiGetUIwidth();

float prwuiGetUIheight();

float prwuiGetUIScaleFactor();

void prwuiPushStack();

void prwuiScale(float x, float y);

void prwuiTranslate(float x, float y);

void prwuiRotate(float x, float y, float z);

void prwuiMult(mat4 val);

void prwuiPopStack();

vec4* prwuiGetModelView();