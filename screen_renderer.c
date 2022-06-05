#include "screen_renderer.h"

#include "ui_render.h"
#include "widget.h"
#include "animation.h"
#include "input.h"
#include "shaders.h"
#include "mesh.h"
#include "https_fetcher.h"

//screen views
#include "title_view.h"
#include "about_view.h"
#include "downloads_view.h"

#include <stdio.h>
#include <string.h>

#include <cglm/cglm.h>

#ifndef EMSCRIPTEN
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#endif

static int m_pageSelected = 0;

static PRWwidget* titleTab = NULL;
static PRWwidget* otherTab = NULL;
static PRWwidget* downloadTab = NULL;

static PRWsmoother m_tabSelectorX1;
static PRWsmoother m_tabSelectorX2;

static PRWtimer m_timer;
static float m_tabsWidth = 250;

void i_drawTabs()
{
    prwuiPushStack();

    float uiWidth = prwuiGetUIwidth();
    float selectorX1 = prwaSmootherValue(&m_tabSelectorX1);
    float selectorX2 = prwaSmootherValue(&m_tabSelectorX2);

    if(uiWidth < m_tabsWidth * 1.5f)
    {
        float translatex = (selectorX1 + selectorX2) / 2;
        translatex -= uiWidth / 2;
        prwuiTranslate(-translatex, 0);
    }

    prwuiGenGradientQuad(PRWUI_TO_BOTTOM, selectorX1, 0, selectorX2, NAV_BAR_HEIGHT, -16741121, -16749608, 0);

    m_tabsWidth = NAV_BAR_HEIGHT;
    PRWwidget* tabs[] = { titleTab, otherTab, downloadTab };
    for(int i = 0; i < 3; i++)
    {
        PRWwidget* tab = tabs[i];
        tab->x = m_tabsWidth;
        m_tabsWidth += tab->width + NAV_BAR_HEIGHT;

        tab->height = NAV_BAR_HEIGHT;
        prwwWidgetDraw(tab);
        prwuiGenQuad(tab->x, tab->y, tab->x + tab->width, tab->y + tab->height, 1275068416, 0);
        int textColor = prwwWidgetHovered(tab) ? -70 : -2039584;
        prwuiGenString(PRWUI_CENTER, prwwWidgetText(tab), tab->x + tab->width / 2, tab->y + tab->height / 2, textColor);
    }

    prwuiPopStack();

    if(selectorX1 == 0)
    {
        prwaSmootherGrabTo(&m_tabSelectorX1, titleTab->x - MARGIN);
        prwaSmootherGrabTo(&m_tabSelectorX2, titleTab->x + titleTab->width + MARGIN);
    }
}

void i_drawNavigationBar()
{
    float uiWidth = prwuiGetUIwidth();

    prwuiGenQuad(0, 0, uiWidth, NAV_BAR_HEIGHT, -1308622848, 0);
    prwuiGenQuad(0, NAV_BAR_HEIGHT, uiWidth, NAV_BAR_HEIGHT + MARGIN, -16749608, 0);
    prwuiGenGradientQuad(PRWUI_TO_BOTTOM, 0, NAV_BAR_HEIGHT + MARGIN, uiWidth, NAV_BAR_HEIGHT * 2 + MARGIN, 1275068416, 0, 0);

    i_drawTabs();
}

void i_onTitleTab(PRWwidget* widget)
{
    m_pageSelected = 0;
    prwaSmootherGrabTo(&m_tabSelectorX1, widget->x - MARGIN);
    prwaSmootherGrabTo(&m_tabSelectorX2, widget->x + widget->width + MARGIN);
}

void i_onAboutTab(PRWwidget* widget)
{
    m_pageSelected = 1;
    prwaSmootherGrabTo(&m_tabSelectorX1, widget->x - MARGIN);
    prwaSmootherGrabTo(&m_tabSelectorX2, widget->x + widget->width + MARGIN);
}

void i_onThisWebTab(PRWwidget* widget)
{
    m_pageSelected = 2;
    prwaSmootherGrabTo(&m_tabSelectorX1, widget->x - MARGIN);
    prwaSmootherGrabTo(&m_tabSelectorX2, widget->x + widget->width + MARGIN);
}

void prwInitMenuScreen()
{
    titleTab = prwwGenWidget(PRWW_TYPE_WIDGET);
    otherTab = prwwGenWidget(PRWW_TYPE_WIDGET);
    downloadTab = prwwGenWidget(PRWW_TYPE_WIDGET);

    prwwWidgetSetText(titleTab, "Parkour Recorder");
    prwwWidgetSetText(otherTab, "About");
    prwwWidgetSetText(downloadTab, "Downloads");
    titleTab->width = prwuiGetStringWidth(prwwWidgetText(titleTab)) + 10;
    otherTab->width = prwuiGetStringWidth(prwwWidgetText(otherTab)) + 10;
    downloadTab->width = prwuiGetStringWidth(prwwWidgetText(downloadTab)) + 10;
    prwwWidgetSetCallback(titleTab, i_onTitleTab);
    prwwWidgetSetCallback(otherTab, i_onAboutTab);
    prwwWidgetSetCallback(downloadTab, i_onThisWebTab);

    prwaSmootherSetSpeed(&m_tabSelectorX1, 20);
    prwaSmootherSetSpeed(&m_tabSelectorX2, 20);
    prwaSmootherSetAndGrab(&m_tabSelectorX1, 0);
    prwaSmootherSetAndGrab(&m_tabSelectorX2, 20);

    prwaInitTimer(&m_timer);

    i_drawTabs();
}

int prwScreenPage()
{
    return m_pageSelected;
}

float prwScreenPartialTicks()
{
    return prwaTimerPartialTicks(&m_timer);
}

float prwScreenTickLerp(float start, float end)
{
    return prwaTimerLerp(&m_timer, start, end);
}

void prwRenderMenuScreen()
{
    prwuiSetupUIrendering();

    if(prwaTimerTicksPassed(&m_timer))
    {
        prwTickTitleView();
        prwTickAboutView();
        prwTickDownloadsView();
    }

    if(m_pageSelected == 0) prwDrawTitleView();
    else if(m_pageSelected == 1) prwDrawAboutView();
    else if(m_pageSelected == 2) prwDrawDownloadsView();

    prwuiSetupUIrendering();

    i_drawNavigationBar();

    prwuiRenderBatch();
}