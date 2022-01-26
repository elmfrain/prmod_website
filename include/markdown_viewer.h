#pragma once

#include "widget.h"

typedef struct PRWmarkdownViewer
{
    PRWwidget* widget;
} PRWmarkdownViewer;

void prwmdDrawMarkdown(PRWmarkdownViewer* viewer);

PRWmarkdownViewer* prwmdGenMarkdown(const char* url);

void prwmdFreeMarkdown(PRWmarkdownViewer* viewer);