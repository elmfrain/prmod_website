#include "screen_renderer.h"

#include "ui_render.h"
#include "widget.h"
#include "animation.h"
#include "input.h"
#include "shaders.h"
#include "mesh.h"
#include "https_fetcher.h"

#include <stdio.h>
#include <string.h>

#include <cglm/cglm.h>

#include <glad/glad.h>

#define NAV_BAR_HEIGHT 15
#define MARGIN 3

static int m_pageSelected = 0;

static PRWwidget* titleTab = NULL;
static PRWwidget* otherTab = NULL;

static PRWsmoother m_tabSelectorX1;
static PRWsmoother m_tabSelectorX2;

void i_drawTabs()
{
    prwuiGenGradientQuad(PRWUI_TO_BOTTOM, prwaSmootherValue(&m_tabSelectorX1), 0, prwaSmootherValue(&m_tabSelectorX2), NAV_BAR_HEIGHT, -16741121, -16749608, 0);

    float tabPosition = NAV_BAR_HEIGHT;
    PRWwidget* tabs[] = { titleTab, otherTab };
    for(int i = 0; i < 2; i++)
    {
        PRWwidget* tab = tabs[i];
        tab->x = tabPosition;
        tabPosition += tab->width + NAV_BAR_HEIGHT;

        tab->height = NAV_BAR_HEIGHT;
        prwwWidgetDraw(tab);
        prwuiGenQuad(tab->x, tab->y, tab->x + tab->width, tab->y + tab->height, 1275068416, 0);
        int textColor = prwwWidgetHovered(tab) ? -70 : -2039584;
        prwuiGenString(PRWUI_CENTER, prwwWidgetText(tab), tab->x + tab->width / 2, tab->y + tab->height / 2, textColor);
    }

    if(prwaSmootherValue(&m_tabSelectorX1) == 0)
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

void i_onOtherTab(PRWwidget* widget)
{
    m_pageSelected = 1;
    prwaSmootherGrabTo(&m_tabSelectorX1, widget->x - MARGIN);
    prwaSmootherGrabTo(&m_tabSelectorX2, widget->x + widget->width + MARGIN);
}

void prwInitMenuScreen()
{
    titleTab = prwwGenWidget(PRWW_TYPE_WIDGET);
    otherTab = prwwGenWidget(PRWW_TYPE_WIDGET);

    prwwWidgetSetText(titleTab, "Parkour Recorder");
    prwwWidgetSetText(otherTab, "Other");
    titleTab->width = prwuiGetStringWidth(prwwWidgetText(titleTab)) + 10;
    otherTab->width = prwuiGetStringWidth(prwwWidgetText(otherTab)) + 10;
    prwwWidgetSetCallback(titleTab, i_onTitleTab);
    prwwWidgetSetCallback(otherTab, i_onOtherTab);

    prwaSmootherSetSpeed(&m_tabSelectorX1, 20);
    prwaSmootherSetSpeed(&m_tabSelectorX2, 20);
    prwaSmootherSetAndGrab(&m_tabSelectorX1, 0);
    prwaSmootherSetAndGrab(&m_tabSelectorX2, 20);

    i_drawTabs();
}

static PRWwidget* m_changeLogVP = NULL;
static PRWwidget* m_changeLogButtons[2] = { 0 };
static PRWfetcher* m_changelogFetcher = NULL;
static PRWsmoother m_changelogScroll;
static PRWsmoother m_logoFov;
static PRWsmoother m_logoAngleSpd;
static PRWtimer m_timer;
static int m_logoOpacityCtr = 0;
static float m_logoAngle = -90.0f;
static float m_pLogoAngle = -90.0f;
static int m_userCtrlCtr = 40;


static void i_drawChangeLog()
{
    float uiWidth = prwuiGetUIwidth();
    float uiHeight = prwuiGetUIheight();
    m_changeLogVP->x = uiWidth / 4 - 5;
    m_changeLogVP->y = uiHeight * 0.75f;
    m_changeLogVP->width = (uiWidth * 3 / 4 + 5) - m_changeLogVP->x;
    m_changeLogVP->height = uiHeight - m_changeLogVP->y;
    prwwViewportStart(m_changeLogVP, 0);
    {
        prwuiGenQuad(0, 0, m_changeLogVP->width, m_changeLogVP->height, 2130706432, 0);
        prwuiGenQuad(0, 0, m_changeLogVP->width, 12, 1711276032, 0);
        prwuiGenString(PRWUI_MID_LEFT, "Changelog", 6, 6, -1);

        m_changeLogButtons[0]->x = m_changeLogButtons[1]->x = m_changeLogVP->width + MARGIN;
        m_changeLogButtons[1]->y = m_changeLogVP->height - m_changeLogButtons[1]->height;
        prwwWidgetDraw(m_changeLogButtons[0]);
        prwwWidgetDraw(m_changeLogButtons[1]);
    }
    prwwViewportEnd(m_changeLogVP);

    if(prwwWidgetJustPressed(m_changeLogButtons[0]))
        prwaSmootherGrabTo(&m_changelogScroll, m_changelogScroll.grabbingTo - 30);
    if(prwwWidgetJustPressed(m_changeLogButtons[1]))
        prwaSmootherGrabTo(&m_changelogScroll, m_changelogScroll.grabbingTo + 30);

    m_changeLogVP->x += 5;
    m_changeLogVP->y += 17;
    m_changeLogVP->width -= 10;
    m_changeLogVP->height -= 20;
    prwwViewportStart(m_changeLogVP, 1);
    {
        if(prwwWidgetHovered(m_changeLogVP) && prwiJustScrolled())
        {
            double prevGrab = prwaSmootherGrabbingTo(&m_changelogScroll);
            prwaSmootherGrabTo(&m_changelogScroll, prevGrab + prwiScrollDeltaY() * 20.0);
        }
        float scroll = prwaSmootherValue(&m_changelogScroll);

        float yCursor = 0;
        prwuiTranslate(0, -scroll);

        int lineStart = 0;
        char line[1024];

        int changelogLen = 0;
        const char* changelog = prwfFetchString(m_changelogFetcher, &changelogLen);
        for(int i = 0; i < changelogLen; i++)
        {
            if(changelog[i] == '\n' || changelog[i] == '\00')
            {
                memset(line, 0, sizeof(line));
                memcpy(line, changelog + lineStart, (i - lineStart) < sizeof(line) ? i - lineStart : sizeof(line) - 1);
                lineStart = i + 1;

                //Split line if it's too long
                if(prwuiGetStringWidth(line) > m_changeLogVP->width && m_changeLogVP->width > 80)
                {
                    char subline[1024] = { 0 };
                    char formatting[64] = { 0 };
                    char prevFormatting[64] ={ 0 };
                    int lineLen = strlen(line);
                    char indents[64] = { 0 }; for(int j = 0; j < lineLen; j++) { if(line[j] == ' ') strcat(indents, " "); else break; };
                    float xCursor = 0;
                    int bytesRead = 0;

                    for(int j = 0, s = 0; j < lineLen;)
                    {
                        int unicode = prwfrUnicodeFromUTF8(&line[j], &bytesRead);
                        xCursor += prwuiGetCharWidth(unicode);

                        if(unicode == 167 && j + bytesRead < lineLen)
                        {
                            strncat(prevFormatting, line + j, 3);
                            j++;
                            continue;
                        }

                        if(j + 1 == lineLen)
                        {
                            strcat(subline, formatting);
                            strncat(subline, line + s, lineLen - s);
                            prwuiGenString(PRWUI_TOP_LEFT, subline, 0, yCursor, -1);
                            yCursor += 10;
                        }
                        else if(m_changeLogVP->width - 30 < xCursor)
                        {
                            strcat(subline, formatting);
                            strncat(subline, line + s, (j - 1) - s);
                            strcat(subline, "-"); 

                            prwuiGenString(PRWUI_TOP_LEFT, subline, 0, yCursor, -1);
                            memset(subline, 0, sizeof(subline));
                            yCursor += 10;

                            char strConcat [sizeof(subline)] = { 0 };
                            strncat(strConcat, line, j - 1);
                            strcat(strConcat, indents);
                            strcat(strConcat, line + (j - 1));
                            memset(line, 0, sizeof(line));
                            strcpy(line, strConcat);
                            lineLen = strlen(line);

                            s = j - 1;
                            xCursor = 0;
                            memset(formatting, 0, sizeof(formatting));
                            strcpy(formatting, prevFormatting);
                        }

                        j += bytesRead;
                    }
                }
                else 
                {
                    prwuiGenString(PRWUI_TOP_LEFT, line, 0, yCursor, -1);
                    yCursor += 10;
                }
            }
        }

        float MAX_SCROLL = yCursor - m_changeLogVP->height / 2;
        if(scroll < 0) prwaSmootherGrabTo(&m_changelogScroll, 0);
        else if(scroll > MAX_SCROLL) prwaSmootherGrabTo(&m_changelogScroll, MAX_SCROLL);
    }
    prwwViewportEnd(m_changeLogVP);
}

PRWmesh* m_modLogoMesh = NULL;
PRWmesh* m_modLogoShadowMesh = NULL;
mat4 m_projectionMatrix;
mat4 m_modelviewMatrix;

static void i_drawTitleView()
{
    if(!m_modLogoMesh)
    {
        prwmLoad("res/3d_logo_baked.bin");
        m_modLogoMesh = prwmMeshGet("pr_logo");
        m_modLogoShadowMesh = prwmMeshGet("shadow_plane");
        prwaInitSmoother(&m_logoFov);
        prwaInitSmoother(&m_logoAngleSpd);
        prwaSmootherSetAndGrab(&m_logoAngleSpd, 1.8);
        m_logoFov.speed = 1.8;
        m_logoAngleSpd.speed = 5;
        prwaSmootherSetValue(&m_logoFov, 1.8);
        prwaSmootherGrabTo(&m_logoFov, 38);
        prwaInitTimer(&m_timer);
    }

    if(!m_changelogFetcher)
        m_changelogFetcher = prwfFetch("elmfer.com", "/parkour_recorder/changelog.txt");

    if(!m_changeLogVP)
    {
        m_changeLogVP = prwwGenWidget(PRWW_TYPE_VIEWPORT);
        m_changeLogButtons[0] = prwwGenWidget(PRWW_TYPE_BUTTON);
        m_changeLogButtons[1] = prwwGenWidget(PRWW_TYPE_BUTTON);
        m_changeLogButtons[0]->width = m_changeLogButtons[1]->width = 30;
        m_changeLogButtons[0]->height = m_changeLogButtons[1]->height = 15;
        prwwWidgetSetText(m_changeLogButtons[0], "Up");
        prwwWidgetSetText(m_changeLogButtons[1], "Down");
        prwaInitSmoother(&m_changelogScroll);
    }
    float uiWidth = prwuiGetUIwidth();
    float uiHeight = prwuiGetUIheight();
    float yCursor = prwuiGetStringHeight() * 4 + 19;

    prwuiGenGradientQuad(PRWUI_TO_BOTTOM, uiWidth / 4 - 10, 15, uiWidth * 3 / 4 + 10, uiHeight * 5 / 6, 1711276032, 0, 0);
    prwuiGenQuad(uiWidth / 3, 18, uiWidth * 2 / 3, yCursor, 1275068416, 0);
    prwuiGenGradientQuad(PRWUI_TO_RIGHT, uiWidth / 4 - 10, 18, uiWidth / 3, yCursor, 0, 1275068416, 0);
    prwuiGenGradientQuad(PRWUI_TO_LEFT, uiWidth * 2 / 3, 18, uiWidth * 3 / 4 + 10, yCursor, 0, 1275068416, 0);

    float titleWidth = prwuiGetStringWidth("Parkour Recorder");
    prwuiPushStack();
    {
        prwuiScale(2, 2);
        prwuiGenString(PRWUI_TOP_CENTER, "Parkour Recorder", uiWidth / 4, 13.5f, -8421505);
        prwuiGenString(PRWUI_TOP_CENTER, "Parkour Recorder", uiWidth / 4, 12.5f, -1);
    }
    prwuiPopStack();

    prwuiGenString(PRWUI_TOP_LEFT, "by elmfer - Website", uiWidth / 2 - titleWidth, yCursor - 12, -8421505);
    prwuiGenString(PRWUI_TOP_LEFT, "by elmfer - Website", uiWidth / 2 - titleWidth, yCursor - 13, -1);

    yCursor += 8;

    prwuiGenString(PRWUI_CENTER, "Record and playback your parkour sessions.", uiWidth / 2, yCursor + 1, -10066330);
    prwuiGenString(PRWUI_CENTER, "Record and playback your parkour sessions.", uiWidth / 2, yCursor, -1);

    i_drawChangeLog();

    glm_mat4_identity(m_projectionMatrix);
    vec3 v = {0, 0.4f, 0}; glm_translate(m_projectionMatrix, v);
    glm_perspective(glm_rad(prwaSmootherValue(&m_logoFov)), prwuiGetUIwidth() / prwuiGetUIheight(), 0.1, 100, m_projectionMatrix);
    glm_mat4_identity(m_modelviewMatrix);
    vec3 v1 = {1, 0, 0}; glm_rotate(m_modelviewMatrix, glm_rad(5.2f), v1);
    vec3 v2 = {0.0f, -1.51f, -10.0f}; glm_translate(m_modelviewMatrix, v2);
    vec3 v3 = {0, 1, 0}; glm_rotate(m_modelviewMatrix, glm_rad(prwaTimerLerp(&m_timer, m_pLogoAngle, m_logoAngle)), v3);

    prwuiRenderBatch();

    if(prwiJustScrolled() && !prwwWidgetHovered(m_changeLogVP))
    {
        m_logoAngle += prwiScrollDeltaY() * 10;
        m_userCtrlCtr = 15;
    }
    if(prwiMButtonJustPressed(GLFW_MOUSE_BUTTON_1) && !prwwWidgetHovered(m_changeLogVP) && (prwiCursorY() / prwuiGetUIScaleFactor() > 15))
    {
        m_userCtrlCtr = -2;
    }
    if(prwiCursorXDelta() != 0 && m_userCtrlCtr == -2) m_userCtrlCtr = -1;
    if(prwiMButtonJustReleased(GLFW_MOUSE_BUTTON_1))
    {
        if(m_userCtrlCtr == -2) m_userCtrlCtr = 40;
        else if(m_userCtrlCtr == - 1) m_userCtrlCtr = 0;
    }
    if(prwaTimerTicksPassed(&m_timer))
    {
        if(m_pageSelected == 0 && m_logoOpacityCtr < 25) m_logoOpacityCtr++;
        m_pLogoAngle = m_logoAngle;
        m_logoAngle += prwaSmootherValue(&m_logoAngleSpd);
        if(m_userCtrlCtr >= 0) m_userCtrlCtr++;

        if(m_userCtrlCtr >= 40 || m_userCtrlCtr == -2) m_logoAngleSpd.grabbingTo = 1.8;
        else if(m_userCtrlCtr == -1) m_logoAngleSpd.grabbingTo = prwiCursorXDelta() * 3;
        else m_logoAngleSpd.grabbingTo = 0;
    }

    prwsSetProjectionMatrix(m_projectionMatrix);
    prwsSetModelViewMatrix(m_modelviewMatrix);
    prwsSetColor(1, 1, 1, m_logoOpacityCtr / 25.0f);
    prws_POS_UV_shader();

    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    prwmMeshRenderv(m_modLogoShadowMesh);
    prwmMeshRenderv(m_modLogoMesh);

    prwsSetColor(1, 1, 1, 1);
}

void prwRenderMenuScreen()
{
    prwuiSetupUIrendering();

    if(m_pageSelected == 0) i_drawTitleView();

    prwuiSetupUIrendering();

    i_drawNavigationBar();

    prwuiRenderBatch();
}