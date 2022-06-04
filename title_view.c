#include "title_view.h"

#include "mesh.h"
#include "widget.h"
#include "ui_render.h"
#include "animation.h"
#include "https_fetcher.h"
#include "screen_renderer.h"
#include "shaders.h"
#include "input.h"

#ifndef EMSCRIPTEN
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#endif
#include <cglm/cglm.h>

#include <string.h>

static PRWwidget* m_changeLogVP = NULL;
static PRWwidget* m_changeLogButtons[2] = { 0 };
static PRWfetcher* m_changelogFetcher = NULL;
static PRWsmoother m_changelogScroll;
static PRWsmoother m_logoFov;
static PRWsmoother m_logoAngleSpd;
static int m_logoOpacityCtr = 0;
static float m_logoAngle = -90.0f;
static float m_pLogoAngle = -90.0f;
static int m_userCtrlCtr = 40;

static PRWmesh* m_modLogoMesh = NULL;
static PRWmesh* m_modLogoShadowMesh = NULL;
static mat4 m_projectionMatrix;
static mat4 m_modelviewMatrix;

static PRWwidget* m_body = NULL;

//String insert
static char strbuffer[2048];
#define strins(dst, src, rep) {strcpy(strbuffer, dst + rep); strcpy(dst, src); strcat(dst, strbuffer);}

static void i_drawChangeLog()
{
    m_changeLogVP->y = m_body->height * 0.75f;
    m_changeLogVP->width = m_body->width - 10;
    m_changeLogVP->height = m_body->height - m_changeLogVP->y;
    m_changeLogVP->x = m_body->width / 2 - m_changeLogVP->width / 2;

    //Draw changelog body
    prwwViewportStart(m_changeLogVP, 0);
    {
        prwuiGenQuad(0, 0, m_changeLogVP->width, m_changeLogVP->height, 2130706432, 0);
        prwuiGenQuad(0, 0, m_changeLogVP->width, 12, 1711276032, 0);
        prwuiGenString(PRWUI_MID_LEFT, "Changelog", 6, 6, -1);

        m_changeLogButtons[0]->height = m_changeLogButtons[1]->height = m_changeLogVP->height * 0.48f;
        m_changeLogButtons[0]->x = m_changeLogButtons[1]->x = m_changeLogVP->width + MARGIN;
        m_changeLogButtons[1]->y = m_changeLogVP->height - m_changeLogButtons[1]->height;

        //Draw scroll buttons
        for(int i = 0; i < 2; i++)
        {
            PRWwidget* b = m_changeLogButtons[i];
            prwwWidgetDraw(b);
            prwuiGenQuad(b->x, b->y, b->x + b->width, b->y + b->height, 1711276032, 0);
            prwuiPushStack();
            {
                float translateX = b->x + b->width / 2;
                float translateY = b->y + b->height / 2;
                prwuiTranslate(translateX, translateY);
                prwuiRotate(0, 0, 90);
                prwuiGenString(PRWUI_CENTER, prwwWidgetText(b), 0, 0, -1);
            }
            prwuiPopStack();
        }
    }
    prwwViewportEnd(m_changeLogVP);

    //Handle manual scrolling (through buttons)
    if(prwwWidgetJustPressed(m_changeLogButtons[0]))
        prwaSmootherGrabTo(&m_changelogScroll, m_changelogScroll.grabbingTo - 30);
    if(prwwWidgetJustPressed(m_changeLogButtons[1]))
        prwaSmootherGrabTo(&m_changelogScroll, m_changelogScroll.grabbingTo + 30);

    //Handle scrolling
    if(prwwWidgetHovered(m_changeLogVP) && prwiJustScrolled())
    {
        double prevGrab = prwaSmootherGrabbingTo(&m_changelogScroll);
        prwaSmootherGrabTo(&m_changelogScroll, prevGrab + prwiScrollDeltaY() * 20.0);
    }

    m_changeLogVP->x += 5;
    m_changeLogVP->y += 17;
    m_changeLogVP->width -= 10;
    m_changeLogVP->height -= 20;
    prwwViewportStart(m_changeLogVP, 1);

    float uiScale = prwuiGetUIScaleFactor();
    float scroll = (int) (prwaSmootherValue(&m_changelogScroll) * uiScale);
    scroll /= uiScale;
    prwuiTranslate(0, -scroll);
    const float SPACE_WIDTH = prwuiGetCharWidth(' ');
    int lineStart = 0;
    int changelogLen = 0;
    float yCursor = 0;
    const char* changelog = prwfFetchString(m_changelogFetcher, &changelogLen);
    for(int i = 0; i < changelogLen; i++)
    {
        //New line found
        if(changelog[i] == '\n' || changelog[i] == '\00')
        {
            //Line strings and states
            char line[2048] = {0};
            float xCursor = 0;
            memset(line, 0, sizeof(line));
            memcpy(line, changelog + lineStart, (i - lineStart) < sizeof(line) ? i - lineStart : sizeof(line) - 1);
            lineStart = i + 1;
            //Subline string and states
            int linePos = 0;
            int lineLen = strlen(line);
            char subline[4096] = {0};
            char f[512] = {0};
            //Get indent spacing
            int j;
            for(j = 0; line[j] == ' '; j++, lineLen--) xCursor += SPACE_WIDTH;
        subLine:
            prwfrGetFormats(f, subline);
            linePos += prwfrSplitToFitr(subline, line + linePos + j, m_changeLogVP->width - xCursor, " ");
            strins(subline, f, 0);
            //Draw string
            prwuiGenString(PRWUI_TOP_LEFT, subline, xCursor, yCursor, -1);
            yCursor += 10;
            //If this line is cut and still has more text to show
            if(linePos < lineLen) goto subLine;
        }
    }
    float MAX_SCROLL = yCursor - m_changeLogVP->height / 2;
    if(prwwWidgetJustPressed(m_changeLogVP))
    {
        if(prwwWidgetLCursorY(m_changeLogVP) < m_changeLogVP->height / 2) m_changelogScroll.grabbingTo -= 35;
        else m_changelogScroll.grabbingTo += 35;
    }
    if(MAX_SCROLL < 0) MAX_SCROLL = 0;
    if(scroll < 0) prwaSmootherGrabTo(&m_changelogScroll, 0);
    else if(scroll > MAX_SCROLL) prwaSmootherGrabTo(&m_changelogScroll, MAX_SCROLL);
    
    prwwViewportEnd(m_changeLogVP);
}

void prwTickTitleView()
{
    if(prwScreenPage() == 0 && m_logoOpacityCtr < 25) m_logoOpacityCtr++;
    m_pLogoAngle = m_logoAngle;
    m_logoAngle += prwaSmootherValue(&m_logoAngleSpd);
    if(m_userCtrlCtr >= 0) m_userCtrlCtr++;
    if(m_userCtrlCtr >= 40 || m_userCtrlCtr == -2) m_logoAngleSpd.grabbingTo = 1.8;
    else if(m_userCtrlCtr == -1) m_logoAngleSpd.grabbingTo = prwiCursorXDelta() * 3;
    else m_logoAngleSpd.grabbingTo = 0;
}

void prwDrawTitleView()
{
    if(!m_modLogoMesh)
    {
        prwmLoad("res/3d_logo_baked.obj");
        m_modLogoMesh = prwmMeshGet("pr_logo");
        m_modLogoShadowMesh = prwmMeshGet("shadow_plane");

        prwvfVTXFMT meshVtxFmt;
        meshVtxFmt[0] = 2;

        meshVtxFmt[1] = PRWVF_ATTRB_USAGE_POS
                      | PRWVF_ATTRB_TYPE_FLOAT
                      | PRWVF_ATTRB_SIZE(3)
                      | PRWVF_ATTRB_NORMALIZED_FALSE;

        meshVtxFmt[2] = PRWVF_ATTRB_USAGE_UV
                      | PRWVF_ATTRB_TYPE_FLOAT
                      | PRWVF_ATTRB_SIZE(2)
                      | PRWVF_ATTRB_NORMALIZED_FALSE;

        prwmMakeRenderable(m_modLogoMesh, meshVtxFmt);
        prwmMakeRenderable(m_modLogoShadowMesh, meshVtxFmt);

        prwaInitSmoother(&m_logoFov);
        prwaInitSmoother(&m_logoAngleSpd);
        prwaSmootherSetAndGrab(&m_logoAngleSpd, 1.8);
        m_logoFov.speed = 1.8;
        m_logoAngleSpd.speed = 5;
        prwaSmootherSetValue(&m_logoFov, 1.8);
        prwaSmootherGrabTo(&m_logoFov, 38);
    }

    if(!m_changelogFetcher)
        m_changelogFetcher = prwfFetchURL("https://prmod.elmfer.com/changelog.txt");

    if(!m_body)
        m_body = prwwGenWidget(PRWW_TYPE_VIEWPORT);

    if(!m_changeLogVP)
    {
        m_changeLogVP = prwwGenWidget(PRWW_TYPE_VIEWPORT);
        m_changeLogButtons[0] = prwwGenWidget(PRWW_TYPE_WIDGET);
        m_changeLogButtons[1] = prwwGenWidget(PRWW_TYPE_WIDGET);
        m_changeLogButtons[0]->width = m_changeLogButtons[1]->width = 13;
        m_changeLogButtons[0]->height = m_changeLogButtons[1]->height = 15;
        prwwWidgetSetText(m_changeLogButtons[0], "<<");
        prwwWidgetSetText(m_changeLogButtons[1], ">>");
        prwaInitSmoother(&m_changelogScroll);
    }
    float uiWidth = prwuiGetUIwidth();
    float uiHeight = prwuiGetUIheight();
    float yCursor = prwuiGetStringHeight() * 4 + 19;

    m_body->width = uiWidth / 2 + 20;
    if(m_body->width < 250) m_body->width = 250;
    m_body->height = uiHeight - NAV_BAR_HEIGHT;
    if(m_body->height < 300) m_body->height = 300;
    m_body->x = uiWidth / 2 - m_body->width / 2;
    m_body->y = NAV_BAR_HEIGHT;

    prwwViewportStart(m_body, 0);
    {
        prwuiGenGradientQuad(PRWUI_TO_BOTTOM, 0, 0, m_body->width, m_body->height * 5 / 6, 1711276032, 0, 0);
        prwuiGenQuad(m_body->width / 5, 0, m_body->width * 4 / 5, 40, 1275068416, 0);
        prwuiGenGradientQuad(PRWUI_TO_RIGHT, 0, 0, m_body->width / 5, 40, 0, 1275068416, 0);
        prwuiGenGradientQuad(PRWUI_TO_LEFT, m_body->width * 4 / 5, 0, m_body->width, 40, 0, 1275068416, 0);
    }
    prwwViewportEnd(m_body);

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

    prwwViewportStart(m_body, 0);
    i_drawChangeLog();
    prwwViewportEnd(m_body);

    glm_mat4_identity(m_projectionMatrix);
    vec3 v = {0, 0.4f, 0}; glm_translate(m_projectionMatrix, v);
    glm_perspective(glm_rad(prwaSmootherValue(&m_logoFov)), prwuiGetUIwidth() / prwuiGetUIheight(), 0.1, 100, m_projectionMatrix);
    glm_mat4_identity(m_modelviewMatrix);
    vec3 v1 = {1, 0, 0}; glm_rotate(m_modelviewMatrix, glm_rad(5.2f), v1);
    vec3 v2 = {0.0f, -1.51f, -10.0f}; glm_translate(m_modelviewMatrix, v2);
    vec3 v3 = {0, 1, 0}; glm_rotate(m_modelviewMatrix, glm_rad(prwScreenTickLerp(m_pLogoAngle, m_logoAngle)), v3);

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