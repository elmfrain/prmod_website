#include "downloads_view.h"

#include "markdown_viewer.h"
#include "ui_render.h"
#include "screen_renderer.h"
#include "mesh.h"
#include "shaders.h"

#include <stdlib.h>
#include <math.h>
#include <cglm/cglm.h>

#define NAV_BAR_HEIGHT 15

static bool m_hasInit = false;

static PRWwidget* m_body;

static PRWmesh* m_arrowMesh;
static int m_arrowTicks;

static PRWmarkdownViewer* m_titleMD;
static PRWmarkdownViewer* m_noteMD;

static void i_initView();
static void i_drawArrow(float left, float top, float right, float bottom);
static void i_genPIPPerspectiveMatrix(float left, float top, float right, float bottom, float fovy, float nearZ, float farZ, vec4* dest);

void prwTickDownloadsView()
{
    if(prwScreenPage() == 2)
    {
        m_arrowTicks++;
    }
    else
    {
        m_arrowTicks = 0;
    }
}

void prwDrawDownloadsView()
{
    if(!m_hasInit)
    {
        i_initView();
    }

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

    prwwViewportStart(m_body, 0);
    {
        m_titleMD->widget->width = m_body->width - 50;
        m_titleMD->widget->height = 120.0f;
        prwmdDrawMarkdown(m_titleMD);

        prwuiGenHorizontalLine(105.0f, 0, m_body->width, 0xFF444444);

        m_noteMD->widget->y = 105;
        m_noteMD->widget->width = m_body->width;
        m_noteMD->widget->height = 60;
        prwmdDrawMarkdown(m_noteMD);
    }
    prwwViewportEnd(m_body);

    prwwViewportStart(m_body, 0);
    {
        i_drawArrow(m_body->width - 50, 0, m_body->width, 100);
    }
    prwwViewportEnd(m_body);
}

static void i_initView()
{
    m_body = prwwGenWidget(PRWW_TYPE_VIEWPORT);
    m_body->y = NAV_BAR_HEIGHT;

    prwvfVTXFMT arrVtxFmt;

    arrVtxFmt[0] = 2; // Two vertex attributes

    arrVtxFmt[1] = PRWVF_ATTRB_USAGE_POS   // Position attribute
                 | PRWVF_ATTRB_TYPE_FLOAT
                 | PRWVF_ATTRB_SIZE(3)
                 | PRWVF_ATTRB_NORMALIZED_FALSE;

    arrVtxFmt[2] = PRWVF_ATTRB_USAGE_COLOR   // Color attribute
                 | PRWVF_ATTRB_TYPE_FLOAT
                 | PRWVF_ATTRB_SIZE(4)
                 | PRWVF_ATTRB_NORMALIZED_FALSE;

    prwmLoad("res/arrow.ply");
    m_arrowMesh = prwmMeshGet("arrow");
    prwmMakeRenderable(m_arrowMesh, arrVtxFmt);

    m_titleMD = prwmdGenMarkdownFile("res/download.md");
    m_noteMD = prwmdGenMarkdownFile("res/download_note.md");

    m_hasInit = true;
}

static void i_drawArrow(float left, float top, float right, float bottom)
{
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);

    float arrowTicks = m_arrowTicks + prwScreenPartialTicks();
    float arrowAngle = (60.0f * logf(2.0f * arrowTicks + 1.0f) + arrowTicks) * 2.0f;
    float arrowScale = powf(glm_min(arrowTicks, 60.0f) - 60.0f, 4) / 1.296e7f + 0.5f;
    float arrowY = sinf(arrowTicks * GLM_PIf / 20.0f) * 0.15f + 0.05f;
    float arrowOpacity = glm_min(arrowTicks / 25.0f, 1.0f);
    arrowOpacity *= arrowOpacity;

    mat4 projectionMatrix;
    mat4 modelviewMatrix;

    i_genPIPPerspectiveMatrix(left, top, right, bottom, glm_rad(5.0f), 0.1f, 100.0f, projectionMatrix);

    glm_mat4_identity(modelviewMatrix);
    vec3 v1 = {0.0f, arrowY, -10.0f}; glm_translate(modelviewMatrix, v1);
    vec3 v2 = {arrowScale, arrowScale, arrowScale}; glm_scale(modelviewMatrix, v2);
    vec3 v3 = {1, 0, 0}; glm_rotate(modelviewMatrix, -0.0383972f, v3);
    vec3 v4 = {0, 1, 0}; glm_rotate(modelviewMatrix, glm_rad(arrowAngle), v4);

    prwsSetProjectionMatrix(projectionMatrix);
    prwsSetModelViewMatrix(modelviewMatrix);
    prwsSetColor(1, 1, 1, arrowOpacity);
    prws_POS_COLOR_shader();

    prwmMeshRenderv(m_arrowMesh);

    prwsSetColor(1, 1, 1, 1);
}

static void i_genPIPPerspectiveMatrix(float left, float top, float right, float bottom, float fovy, float nearZ, float farZ, vec4* dest)
{
    float uiWidth = prwuiGetUIwidth();
    float uiHeight = prwuiGetUIheight();
    float width = right - left;
    float height = bottom - top;
    float centerX = left + width / 2.0f;
    float centerY = top + height / 2.0f;

    glm_ortho(0, uiWidth, uiHeight, 0.0f, 1.0f, -1.0f, dest);
    glm_mat4_mul(dest, prwuiGetModelView(), dest);
    vec3 v0 = {centerX, centerY, 0.0f}; glm_translate(dest, v0);
    vec3 v1 = {width / 4.0f, -height / 4.0f, 1.0f}; glm_scale(dest, v1);
    mat4 perspective; glm_perspective(fovy, width / height, nearZ, farZ, perspective);
    glm_mat4_mul(dest, perspective, dest);
}