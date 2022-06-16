#pragma once

#define NAV_BAR_HEIGHT 15
#define MARGIN 3

int prwScreenPage();

int prwScreenTicksElapsed();

float prwScreenPartialTicks();

float prwScreenTickLerp(float start, float end);

void prwInitMenuScreen();

void prwRenderMenuScreen();