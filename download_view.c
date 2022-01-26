#include "download_view.h"

#include "markdown_viewer.h"
#include "ui_render.h"

#include "screen_renderer.h"

#include <stdlib.h>

#define NAV_BAR_HEIGHT 15

void prwTickDownloadView()
{

}

static PRWwidget* m_body = NULL;
static PRWmarkdownViewer* m_pageViewer = NULL;

void prwDrawDownloadView()
{
    if(!m_body)
    {
        m_body = prwwGenWidget(PRWW_TYPE_VIEWPORT);
        m_body->y = NAV_BAR_HEIGHT;
    }

    if(!m_pageViewer)
        m_pageViewer = prwmdGenMarkdown("https://elmfer.com/parkour_recorder/download.md");

    float uiWidth = prwuiGetUIwidth();
    float uiHeight = prwuiGetUIheight();

    m_body->width = uiWidth * 2 / 3;
    if(m_body->width < 250) m_body->width = 250;
    m_body->height = uiHeight - NAV_BAR_HEIGHT;
    m_body->x = uiWidth / 2 - m_body->width / 2;

    prwwViewportStart(m_body, 0);
    {
        prwuiGenGradientQuad(PRWUI_TO_RIGHT, -MARGIN, 0, 0, m_body->height, 0, 1275068416, 0);
        prwuiGenGradientQuad(PRWUI_TO_BOTTOM, 0, 0, m_body->width, m_body->height, -869915098, -872415232, 0);
        prwuiGenGradientQuad(PRWUI_TO_RIGHT, m_body->width, 0, m_body->width + MARGIN, m_body->height, 1275068416, 0, 0);
    }
    prwwViewportEnd(m_body);

    m_body->x += NAV_BAR_HEIGHT * 2;
    m_body->width -= NAV_BAR_HEIGHT * 4;

    if(!m_pageViewer) return;

    m_pageViewer->widget->width = m_body->width - NAV_BAR_HEIGHT * 2;
    m_pageViewer->widget->height = m_body->height;
    m_pageViewer->widget->x = NAV_BAR_HEIGHT;

    prwwViewportStart(m_body, 0);
    {
        prwmdDrawMarkdown(m_pageViewer);
    }
    prwwViewportEnd(m_body);
}