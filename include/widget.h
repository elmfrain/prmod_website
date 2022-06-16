#pragma once

#define PRWW_TYPE_WIDGET 0
#define PRWW_TYPE_BUTTON 1
#define PRWW_TYPE_VIEWPORT 2

typedef struct PRWwidget
{
    float x, y, width, height;
} PRWwidget;

typedef void (*PRWwidgetClickedCallback)(PRWwidget*);

PRWwidget* prwwGenWidget(int widgetType);

//Object

void prwwDeleteWidget(PRWwidget* widget);

int prwwWidgetHovered(PRWwidget* widget);

void prwwWidgetTick(PRWwidget* widget);

void prwwWidgetDraw(PRWwidget* widget);

int prwwWidgetVisible(PRWwidget* widget);

int prwwWidgetEnabled(PRWwidget* widget);

float prwwWidgetLCursorX(PRWwidget* widget);

float prwwWidgetLCursorY(PRWwidget* widget);

void prwwWidgetSetEnabled(PRWwidget* widget, int enabled);

void prwwWidgetSetVisible(PRWwidget* widget, int visible);

int prwwWidgetZLevel(PRWwidget* widget);

void prwwWidgetSetZLevel(PRWwidget* widget, int zLevel);

int prwwWidgetIsOnCurrentZLevel(PRWwidget* widget);

int prwwWidgetJustPressed(PRWwidget* widget);

int prwwWidgetIsPressed(PRWwidget* widget);

int prwwWidgetJustReleased(PRWwidget* widget);

void prwwViewportStart(PRWwidget* widget, int clipping);

void prwwViewportEnd(PRWwidget* widget);

PRWwidgetClickedCallback prwwWidgetCallback(PRWwidget* widget);

void prwwWidgetSetCallback(PRWwidget* widget, PRWwidgetClickedCallback callback);

char* prwwWidgetText(PRWwidget* widget);

void prwwWidgetSetText(PRWwidget* widget, const char* str);

int prwwWidgetType(PRWwidget* widget);

//Static

int prwwZLevel();

int prwwSetZLevel(int zLevel);