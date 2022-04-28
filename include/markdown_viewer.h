#pragma once

#include "widget.h"

typedef struct PRWmarkdownViewer
{
    PRWwidget* widget;
} PRWmarkdownViewer;

void prwmdDrawMarkdown(PRWmarkdownViewer* viewer);

PRWmarkdownViewer* prwmdGenMarkdownURL(const char* url);

PRWmarkdownViewer* prwmdGenMarkdownFile(const char* filePath);

void prwmdFreeMarkdown(PRWmarkdownViewer* viewer);