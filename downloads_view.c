#include "downloads_view.h"

#include "markdown_viewer.h"
#include "ui_render.h"
#include "screen_renderer.h"
#include "mesh.h"
#include "shaders.h"
#include "animation.h"
#include "input.h"
#include "https_fetcher.h"

#include <stdlib.h>
#include <math.h>
#include <cglm/cglm.h>
#include <string.h>
#include <json_object.h>
#include <json_tokener.h>
#include <time.h>
#include <locale.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#define NAV_BAR_HEIGHT 15
#define FILE_QUERY_LIST_MAX_SIZE 256
#define FILE_QUERY_HEIGHT 36

typedef struct FileQuery
{
    PRWwidget* m_viewport;
    PRWsmoother m_iconSmoother;
    PRWsmoother m_transition;
    PRWwidget* m_linkButton;
    PRWwidget* m_directButton;

    PRWfetcher* m_linkFetcher;

    // Metadata
    uint32_t m_fileID;
    char m_downloads[128];
    char m_fileSize[128];
    char m_dateReleased[128];
    char m_modVersion[128];
    char m_mcVersion[128];
} FileQuery;

static bool m_hasInit = false;

static PRWwidget* m_body;
static PRWsmoother m_scroll;

static PRWmesh* m_arrowMesh;
static int m_arrowTicks;

static PRWmarkdownViewer* m_titleMD;
static PRWmarkdownViewer* m_noteMD;

static size_t m_fileQueryListSize = 0;
static FileQuery* m_fileQueryList[FILE_QUERY_LIST_MAX_SIZE];
static PRWfetcher* m_fileListFetcher = NULL;
static bool m_listLoaded = false;

static void i_initView();
static void i_drawArrow(float left, float top, float right, float bottom);
static void i_genPIPPerspectiveMatrix(float left, float top, float right, float bottom, float fovy, float nearZ, float farZ, vec4* dest);
static void i_drawTable(float left, float top, float right, float bottom);
static void i_drawText(const char* str, float x, float y, float lineMargin, uint32_t color);
static void i_getFileQueries();
static void i_clearFileQueries();
static FileQuery* i_genFileQuery(json_object* jsonQuery, float transitionDelay);
static void i_drawFileQuery(FileQuery* FileQuery, float width, float* columnWidths, bool altColor);
static void i_deleteFileQuery(FileQuery* fileQuery);
static void i_drawLoadBar(float x, float y);

void prwTickDownloadsView()
{
    if(prwScreenPage() == 2)
    {
        if(!m_listLoaded)
        {
            i_getFileQueries();
        }
        m_arrowTicks++;
    }
    else
    {
        if(m_fileQueryListSize)
        {
            m_listLoaded = false;
            i_clearFileQueries();
        }
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
    float scroll = prwaSmootherValue(&m_scroll);

    m_body->width = uiWidth * 2 / 3;
    if(m_body->width < 380) m_body->width = 380;
    m_body->height = uiHeight - NAV_BAR_HEIGHT;
    m_body->x = uiWidth / 2 - m_body->width / 2;

    prwwViewportStart(m_body, 0);
    {
        prwuiGenGradientQuad(PRWUI_TO_RIGHT, -MARGIN, 0, 0, m_body->height, 0, 1275068416, 0);
        prwuiGenGradientQuad(PRWUI_TO_BOTTOM, 0, 0, m_body->width, m_body->height, -869915098, -872415232, 0);
        prwuiGenGradientQuad(PRWUI_TO_RIGHT, m_body->width, 0, m_body->width + MARGIN, m_body->height, 1275068416, 0, 0);
    }
    prwwViewportEnd(m_body);

    if(prwwWidgetHovered(m_body) && prwiJustScrolled())
    {
        prwaSmootherGrabTo(&m_scroll, m_scroll.grabbingTo + prwiScrollDeltaY() * 35);
    }

    m_body->x += NAV_BAR_HEIGHT * 2;
    m_body->width -= NAV_BAR_HEIGHT * 4;
    float tableHeight = glm_max(12 + FILE_QUERY_HEIGHT, 12 + m_fileQueryListSize * FILE_QUERY_HEIGHT);

    prwwViewportStart(m_body, 0);
    {
        prwuiTranslate(0, -scroll);

        m_titleMD->widget->width = m_body->width - 50;
        m_titleMD->widget->height = 120.0f;
        prwmdDrawMarkdown(m_titleMD);

        i_drawArrow(m_body->width - 50, 0, m_body->width, 100);

        prwuiGenHorizontalLine(105.0f, 0, m_body->width, 0xFF444444);

        m_noteMD->widget->y = 105;
        m_noteMD->widget->width = m_body->width;
        m_noteMD->widget->height = 60;
        prwmdDrawMarkdown(m_noteMD);

        i_drawTable(0, 155, m_body->width, 155 + tableHeight);
    }
    prwwViewportEnd(m_body);

    float MAX_SCROLL = (155 + tableHeight) - (155 + tableHeight) * 0.66f;
    if(MAX_SCROLL < 0) MAX_SCROLL = 0;
    if(scroll < 0) prwaSmootherGrabTo(&m_scroll, 0.5);
    else if(scroll > MAX_SCROLL) prwaSmootherGrabTo(&m_scroll, MAX_SCROLL - 0.5);
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

    // Icons
    prwmLoad("res/icons/jar.ply");
    prwmLoad("res/icons/pr_icon.ply");
    prwmLoad("res/icons/mc_icon.ply");
    prwmLoad("res/icons/download.ply");
    prwmLoad("res/icons/curseforge.ply");

    // Load Arrow
    prwmLoad("res/arrow.ply");
    m_arrowMesh = prwmMeshGet("arrow");
    prwmMakeRenderable(m_arrowMesh, arrVtxFmt);

    prwaInitSmoother(&m_scroll);

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
    float arrowY = sinf(arrowTicks * GLM_PIf / 20.0f) * 0.135f + 0.05f;
    float arrowOpacity = glm_min(arrowTicks / 25.0f, 1.0f);
    arrowOpacity *= arrowOpacity;

    mat4 prevProjectionMatrix;
    mat4 prevModelviewMatrix;
    mat4 projectionMatrix;
    mat4 modelviewMatrix;

    glm_mat4_copy(prwsGetProjectionMatrix(), prevProjectionMatrix);
    glm_mat4_copy(prwsGetModelViewMatrix(), prevModelviewMatrix);

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

    prwsSetProjectionMatrix(prevProjectionMatrix);
    prwsSetModelViewMatrix(prevModelviewMatrix);
    prwsSetColor(1, 1, 1, 1);

    glDisable(GL_DEPTH_TEST);
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

static void i_drawTable(float left, float top, float right, float bottom)
{
    float width = right - left;
    float height = bottom - top;
    float columnWidths[] = { 0.08f, 0.24f, 0.44f, 0.24f};
    char* columnNames[] = {"File", "Version", "Information", "Download"};

    prwuiGenGradientQuad(PRWUI_TO_BOTTOM, left, top, right, bottom, 0xDD5A5A5A, 0xDD333333, 0);
    prwuiGenHorizontalLine(top - 1, left -1 , right, 0xFF888888);
    prwuiGenVerticalLine(left - 1, top - 1, bottom, 0xFF888888);
    prwuiGenVerticalLine(right, top - 1, bottom, 0xFF808080);
    prwuiGenHorizontalLine(bottom, left - 1, right + 1, 0xFF808080);

    float x = left;

    for(int i = 0; i < 4; i++)
    {
        float columnWidth = columnWidths[i] * width;
        uint32_t color = i % 2 == 0 ? 0x7D000000 : 0x9D000000;
        prwuiGenQuad(x, top, x + columnWidth, top + 12, color, 0);
        prwuiGenString(PRWUI_MID_LEFT, columnNames[i], x + 4, top + 6, 0xFFFFFFFF);
        x += columnWidth;

        if(i != 3)
        {
            prwuiGenVerticalLine(x - 1, top + 12, bottom, 0xFF686868);
            prwuiGenVerticalLine(x, top + 12, bottom, 0xFF393939);
            prwuiGenVerticalLine(x + 1, top + 12, bottom, 0xFF686868);
        }
    }

    prwuiPushStack();
    prwuiTranslate(0, top + 12);
    prwuiRenderBatch();

    if(m_fileQueryListSize == 0)
    {
        i_drawLoadBar(left + width / 2, (height - 12) / 2);
    }

    for(int i = 0; i < m_fileQueryListSize; i++)
    {
        i_drawFileQuery(m_fileQueryList[i], m_body->width, columnWidths, i % 2 == 0);
        prwuiTranslate(0, FILE_QUERY_HEIGHT);
    }

    prwuiPopStack();
}

static void i_drawText(const char* str, float x, float y, float lineMargin, uint32_t color)
{
    const char* text = str;

    char line[1024] = {0};
    char* lineEnd;
    int strLen = strlen(str);

    float height = 0.0f;

    while ((lineEnd = strstr(str, "\n")) != NULL)
    {
        height += prwuiGetStringHeight() + lineMargin;

        str = lineEnd + 1;
    }
    height -= lineMargin;

    prwuiPushStack();
    prwuiTranslate(0, -height / 2);

    float cursorY = y;
    str = text;
    while ((lineEnd = strstr(str, "\n")) != NULL)
    {
        memcpy(line, str, lineEnd - str);

        prwuiGenString(PRWUI_TOP_LEFT, line, x, cursorY, color);
        cursorY += prwuiGetStringHeight() + lineMargin;

        memset(line, 0, sizeof(line));
        str = lineEnd + 1;
    }
    
    prwuiPopStack();
}

static void i_getFileQueries()
{
    if(!m_fileListFetcher)
    {
        m_fileListFetcher = prwfFetch("elmfer.xyz", "/api/mod_file_list");
    }

    if(m_fileQueryListSize == 0 && prwfFetchComplete(m_fileListFetcher))
    {
        json_object* jsonObj = json_tokener_parse(prwfFetchString(m_fileListFetcher, NULL));
        int listSize = json_object_array_length(jsonObj);
        listSize = listSize < FILE_QUERY_LIST_MAX_SIZE ? listSize : FILE_QUERY_LIST_MAX_SIZE;

        for(int i = 0; i < listSize; i++)
        {
            json_object* fileQuery = json_object_array_get_idx(jsonObj, i);
            m_fileQueryList[i] = i_genFileQuery(fileQuery, i);
        }
        m_fileQueryListSize = listSize;

        json_object_put(jsonObj);

        prwfFreeFetcher(m_fileListFetcher);
        m_fileListFetcher = NULL;
        m_listLoaded = true;
    }
}

static void i_clearFileQueries()
{
    for(int i = 0; i < m_fileQueryListSize; i++)
    {
        i_deleteFileQuery(m_fileQueryList[i]);
    }

    m_fileQueryListSize = 0;
}

static FileQuery* i_genFileQuery(json_object* jsonQuery, float transitionDelay)
{
    FileQuery* newQuery = malloc(sizeof(FileQuery));

    // Setup Widgets
    newQuery->m_viewport = prwwGenWidget(PRWW_TYPE_VIEWPORT);
    newQuery->m_linkButton = prwwGenWidget(PRWW_TYPE_BUTTON);
    prwwWidgetSetText(newQuery->m_linkButton, "§tdownload§f Link");
    newQuery->m_directButton = prwwGenWidget(PRWW_TYPE_BUTTON);
    prwwWidgetSetText(newQuery->m_directButton, "§tjar§f Direct");
    newQuery->m_linkButton->height = newQuery->m_directButton->height = 14;

    // Setup animation
    prwaInitSmoother(&newQuery->m_iconSmoother);
    prwaSmootherSetAndGrab(&newQuery->m_iconSmoother, 20);
    
    prwaInitSmoother(&newQuery->m_transition);
    prwaSmootherSetValue(&newQuery->m_transition, 0 - powf(2, transitionDelay));
    prwaSmootherGrabTo(&newQuery->m_transition, 1);
    newQuery->m_transition.speed = 5;

    newQuery->m_linkFetcher = NULL;

    // Get file metadata from json
    json_object* fileID = json_object_object_get(jsonQuery, "fileID");
    json_object* downloads = json_object_object_get(jsonQuery, "downloads");
    json_object* fileSize = json_object_object_get(jsonQuery, "fileSize");
    json_object* dateReleased = json_object_object_get(jsonQuery, "dateReleased");
    json_object* modVersion = json_object_object_get(jsonQuery, "modVersion");
    json_object* mcVersion = json_object_object_get(jsonQuery, "mcVersion");

    // Set file metadata
    newQuery->m_fileID = json_object_get_int(fileID);

    setlocale(LC_NUMERIC, "");
    sprintf(newQuery->m_downloads, "%'lu", json_object_get_uint64(downloads));

    uint64_t fqFileSize = json_object_get_uint64(fileSize);
    if(fqFileSize < 1000)
    {
        sprintf(newQuery->m_fileSize, "%luB", fqFileSize);
    }
    else if(fqFileSize < 1000000)
    {
        sprintf(newQuery->m_fileSize, "%.1fKB", fqFileSize / 1000.0);
    }
    else
    {
        sprintf(newQuery->m_fileSize, "%.2fMB", fqFileSize / 1000000.0);
    }

    time_t t = (time_t) json_object_get_uint64(dateReleased);
    struct tm* ptm = localtime(&t);
    strftime(newQuery->m_dateReleased, 127, "%d %b %G", ptm);

    strcpy(newQuery->m_modVersion, json_object_get_string(modVersion));
    strcpy(newQuery->m_mcVersion, json_object_get_string(mcVersion));

    return newQuery;
}

static void i_drawFileQuery(FileQuery* fileQuery, float width, float* columnWidths, bool altColor)
{
    fileQuery->m_viewport->width = m_body->width;
    fileQuery->m_viewport->height = FILE_QUERY_HEIGHT;

    float transiton = glm_max(0, prwaSmootherValue(&fileQuery->m_transition));

    prwwViewportStart(fileQuery->m_viewport, 0);
    {
        prwuiPushStack();
        prwuiTranslate(-5 + transiton * 5, 0);

        float cursorX = 0.0f;
        float fileColW = columnWidths[0] * width;
        float versionColW = columnWidths[1] * width;
        float infoColW = columnWidths[2] * width;
        float downldColW = columnWidths[3] * width;
        uint32_t textColor = 0xFFFFFFFF;
        uint32_t color = altColor ? 0x2A000000 : 0x4A000000;

        prwuiGenQuad(0, 0, fileQuery->m_viewport->width, fileQuery->m_viewport->height, color, 0);
        prwaSmootherGrabTo(&fileQuery->m_iconSmoother, 20);
        if(prwwWidgetHovered(fileQuery->m_viewport))
        {
            prwuiGenGradientQuad(PRWUI_TO_RIGHT, 0, 0, fileQuery->m_viewport->width, fileQuery->m_viewport->height, 0x25FFFFFF, 0x00FFFFFF, 0);
            prwaSmootherGrabTo(&fileQuery->m_iconSmoother, 23);
        }

        prwuiGenIcon("jar", fileColW / 2, fileQuery->m_viewport->height / 2, prwaSmootherValue(&fileQuery->m_iconSmoother), textColor);
        cursorX +=  fileColW;

        char text[1024];
        sprintf(text, "§tpr_icon§7 v%s\n§tmc_icon§7 v%s\n", fileQuery->m_modVersion, fileQuery->m_mcVersion);
        i_drawText(text, cursorX + 6, fileQuery->m_viewport->height / 2, 8, textColor);
        cursorX += versionColW;

        sprintf(text, "§9Released: §7%s\n§9Size: §7%s\n§9Downloads: §7%s\n", fileQuery->m_dateReleased, fileQuery->m_fileSize, fileQuery->m_downloads);
        i_drawText(text, cursorX + 6, fileQuery->m_viewport->height / 2, 2, textColor);
        cursorX += infoColW;

        fileQuery->m_linkButton->width = fileQuery->m_directButton->width = downldColW * 0.9f;
        fileQuery->m_linkButton->x = fileQuery->m_directButton->x = cursorX + downldColW * 0.05f;
        fileQuery->m_linkButton->y = 3;
        fileQuery->m_directButton->y = 19;

        prwwWidgetDraw(fileQuery->m_linkButton);
        prwwWidgetDraw(fileQuery->m_directButton);

        prwuiPopStack();

        if(fileQuery->m_linkFetcher && !prwfFetchComplete(fileQuery->m_linkFetcher))
        {
            i_drawLoadBar(width / 2, FILE_QUERY_HEIGHT / 2);
            prwaSmootherGrabTo(&fileQuery->m_transition, 0.5);
        }
        else
        {
            prwaSmootherGrabTo(&fileQuery->m_transition, 1.0);
        }

        prwsSetColor(1.0f, 1.0f, 1.0f, transiton);
        prwuiRenderBatch();
        prwsSetColor(1.0f, 1.0f, 1.0f, 1.0f);
    }
    prwwViewportEnd(fileQuery->m_viewport);

    if(!fileQuery->m_linkFetcher)
    {
        if(prwwWidgetJustPressed(fileQuery->m_directButton))
        {
            char url[1024];
            sprintf(url, "https://elmfer.xyz/api/get_mod_download?fileID=%d&direct=true", fileQuery->m_fileID);
            fileQuery->m_linkFetcher = prwfFetchURL(url);
        }
        else if(prwwWidgetJustPressed(fileQuery->m_viewport))
        {
            char url[1024];
            sprintf(url, "https://elmfer.xyz/api/get_mod_download?fileID=%d&direct=false", fileQuery->m_fileID);
            fileQuery->m_linkFetcher = prwfFetchURL(url);
        }
    }

    if(fileQuery->m_linkFetcher && prwfFetchComplete(fileQuery->m_linkFetcher))
    {
        char command[3000];
        const char* modUrl = prwfFetchString(fileQuery->m_linkFetcher, NULL);
#ifdef EMSCRIPTEN
        sprintf(command, "window.open(\"%s\");", modUrl);
        emscripten_run_script(command);
#elif defined(__unix__)
        sprintf(command, "xdg-open %s", modUrl);
        system(command);
#elif defined(_WIN32)
        sprintf(command, "start %s", modUrl);
        system(command);
#elif defined(__APPLE__)
        sprintf(command, "open %s", modUrl);
        system(command);
#endif

        prwfFreeFetcher(fileQuery->m_linkFetcher);
        fileQuery->m_linkFetcher = NULL;
    }
}

static void i_deleteFileQuery(FileQuery* fileQuery)
{
    prwwDeleteWidget(fileQuery->m_viewport);
    prwwDeleteWidget(fileQuery->m_linkButton);
    prwwDeleteWidget(fileQuery->m_directButton);

    if(fileQuery->m_linkFetcher)
    {
        prwfFreeFetcher(fileQuery->m_linkFetcher);
    }

    free(fileQuery);
}

static void i_drawLoadBar(float x, float y)
{
    float ticks = (prwScreenTicksElapsed() + prwScreenPartialTicks()) / 4.0f;

    float loadBarSize = 8;
    float cursorX = x - (loadBarSize * 5) / 2;
    float loadBarY = y - loadBarSize / 2;
    for(int i = 0; i < 3; i++, cursorX += loadBarSize * 2)
    {
        uint32_t opacity = (uint32_t) ((sinf(ticks - i * 2) * 0.3f + 0.7f) * 255) << 24;
        prwuiGenQuad(cursorX, loadBarY, cursorX + loadBarSize, loadBarY + loadBarSize, 
                0x00FFFFFF | opacity, 0);
    }
}