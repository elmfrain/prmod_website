#include "widget.h"

#include "shaders.h"
#include "ui_render.h"
#include "input.h"

#include <string.h>
#include <stdio.h>

#define DEFAULT_WIDTH 150
#define DEFAULT_HEIGHT 20

static int m_currentZLevel = 0;

static struct Widget
{
    float x, y, width, height;
    char hovered, visible, enabled;
    int zLevel;
    int type;
    mat4 modelView;

    char justPressed, pressed, released;
    char clipping;
    char text[256];
    PRWwidgetClickedCallback callback;
} Widget;

//Internal Widget list
static struct l_Element
{
    char shouldRemove;
    struct Widget* ptr;
} l_Element;
static int m_listSize = 0;
static int m_listCapacity = 32;
static struct l_Element* m_widgetList = NULL;
static inline void i_lUpdate();
static struct Widget* i_lAdd(struct Widget);
static inline void i_lClear();
static inline void i_lRemove(struct Widget*);

//Viewport clipping
static float m_leftClip = 0;
static float m_topClip = 0;
static float m_rightClip = 0;
static float m_bottomClip = 0;
static mat4 m_clipModelView;
static char m_clipping = 0;

static inline void i_updateHoverState(struct Widget*);

PRWwidget* prwwGenWidget(int widgetType)
{
    struct Widget widget;

    widget.x = widget.y = 0;
    widget.width = DEFAULT_WIDTH;
    widget.height = DEFAULT_HEIGHT;
    widget.visible = widget.enabled = 1;
    widget.hovered = 0;
    widget.zLevel = 0;
    switch(widgetType)
    {
    case PRWW_TYPE_BUTTON:
        widget.type = PRWW_TYPE_BUTTON;
        break;
    case PRWW_TYPE_VIEWPORT:
        widget.type = PRWW_TYPE_VIEWPORT;
        break;
    default:
        widget.type = PRWW_TYPE_WIDGET;
    }

    widget.justPressed = widget.pressed = widget.released = 0;
    widget.clipping = 0;
    memset(widget.text, 0, sizeof(widget.text));
    widget.callback = NULL;

    return (PRWwidget*) i_lAdd(widget);
}

//Object methods

void prwwDeleteWidget(PRWwidget* widget)
{
    i_lRemove((struct Widget*) widget);
}

int prwwWidgetHovered(PRWwidget* widget)
{
    struct Widget* w = (struct Widget*) widget;

    return w->hovered;
}

void prwwWidgetTick(PRWwidget* widget)
{

}

void prwwWidgetDraw(PRWwidget* widget)
{
    struct Widget* w = (struct Widget*) widget;

    glm_mat4_copy(prwsGetModelViewMatrix(), w->modelView);
    glm_mat4_mul(w->modelView, prwuiGetModelView(), w->modelView);
    if(!w->visible) return;

    w->clipping = m_clipping;
    i_updateHoverState(w);

    if(w->type == PRWW_TYPE_BUTTON)
    {
        int textColor = !w->enabled ? -6250336 : w->hovered ? -70 : -2039584;
        int backgroundColor = w->hovered ? 1716276300 : 1711276032;
        prwuiGenQuad(w->x, w->y, w->x + w->width, w->y + w->height, backgroundColor, 0);
        prwuiGenString(PRWUI_CENTER, w->text, w->x + w->width / 2, w->y + w->height / 2, textColor);
    }

    w->justPressed = w->released = 0;
    if(prwiMButtonJustPressed(GLFW_MOUSE_BUTTON_1) && w->hovered)
    {
        w->justPressed = 1;
        w->pressed = 1;
        if(w->callback) w->callback(widget);
    } 
    else if(prwiMButtonJustReleased(GLFW_MOUSE_BUTTON_1) && w->pressed)
    {
        w->pressed = 0;
        w->released = 1;
    }
}

int prwwWidgetVisible(PRWwidget* widget)
{
    struct Widget* w = (struct Widget*) widget;
    return w->visible;
}

int prwwWidgetEnabled(PRWwidget* widget)
{
    struct Widget* w = (struct Widget*) widget;
    return w->enabled;
}

void prwwWidgetSetEnabled(PRWwidget* widget, int enabled)
{
    struct Widget* w = (struct Widget*) widget;
    w->enabled = enabled;
}

void prwwWidgetSetVisible(PRWwidget* widget, int visible)
{
    struct Widget* w = (struct Widget*) widget;
    w->visible = visible;
}

int prwwWidgetZLevel(PRWwidget* widget)
{
    struct Widget* w = (struct Widget*) widget;
    return w->zLevel;
}

void prwwWidgetSetZLevel(PRWwidget* widget, int zLevel)
{
    struct Widget* w = (struct Widget*) widget;
    w->zLevel = zLevel;
}

int prwwWidgetIsOnCurrentZLevel(PRWwidget* widget)
{
    struct Widget* w = (struct Widget*) widget;
    return w->zLevel == m_currentZLevel;
}

int prwwWidgetJustPressed(PRWwidget* widget)
{
    struct Widget* w = (struct Widget*) widget;
    return w->justPressed;
}

int prwwWidgetIsPressed(PRWwidget* widget)
{
    struct Widget* w = (struct Widget*) widget;
    return w->pressed;
}

int prwwWidgetJustReleased(PRWwidget* widget)
{
    struct Widget* w = (struct Widget*) widget;
    return w->released;
}

void prwwViewportStart(PRWwidget* widget, int clipping)
{
    struct Widget* w = (struct Widget*) widget;
    if(w->type != PRWW_TYPE_VIEWPORT) return;

    prwwWidgetDraw(widget);

    if(clipping)
    {
        m_clipping = 1;
        m_leftClip = w->x;
        m_topClip = w->y;
        m_rightClip = w->x + w->width;
        m_bottomClip = w->y + w->height;
        glm_mat4_copy(w->modelView, m_clipModelView);

        prwuiRenderBatch(); // Render and flush the batch before writing to the stencil buffer
        glEnable(GL_STENCIL_TEST);
        glStencilMask(0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        prwuiGenQuad(w->x, w->y, w->x + w->width, w->y + w->height, -1, 0);
        glColorMask(0, 0, 0, 0);
        prwuiRenderBatch();
        glColorMask(1, 1, 1, 1);

        //glStencilMask(0x00);
        glStencilFunc(GL_EQUAL, 1, 0xFF);
    }

    prwuiPushStack();
    prwuiTranslate(w->x, w->y);
}

void prwwViewportEnd(PRWwidget* widget)
{
    struct Widget* w = (struct Widget*) widget;
    if(w->type != PRWW_TYPE_VIEWPORT) return;

    if(m_clipping)
    {
        prwuiRenderBatch();
        glDisable(GL_STENCIL_TEST);
        m_clipping = 0;
    }

    prwuiPopStack();
}

PRWwidgetClickedCallback prwwWidgetCallback(PRWwidget* widget)
{
    struct Widget* w = (struct Widget*) widget;
    return w->callback;
}

void prwwWidgetSetCallback(PRWwidget* widget, PRWwidgetClickedCallback callback)
{
    struct Widget* w = (struct Widget*) widget;
    w->callback = callback;
}

char* prwwWidgetText(PRWwidget* widget)
{
    struct Widget* w = (struct Widget*) widget;
    return w->text;
}

void prwwWidgetSetText(PRWwidget* widget, const char* str)
{
    struct Widget* w = (struct Widget*) widget;

    int strLen = strlen(str);
    int maxCopy = strLen < (sizeof(w->text) - 1) ? strLen : ((sizeof(w->text) - 1));
    memcpy(w->text, str, maxCopy);
}

int prwwWidgetType(PRWwidget* widget)
{
    struct Widget* w = (struct Widget*) widget;
    return w->type;
}

//Static Functions

void prwwTickWidgets()
{
    i_lUpdate();

    for(int i = 0; i < m_listSize; i++)
    {
        struct Widget* widget = m_widgetList[i].ptr;

        prwwWidgetTick((PRWwidget*) widget);
    }
}

void prwwClearWidgets()
{
    i_lClear();
}

int prwwZLevel()
{
    return m_currentZLevel;
}

int prwwSetZLevel(int zLevel)
{
    m_currentZLevel = zLevel;
}

static inline void i_updateHoverState(struct Widget* w)
{
    mat4 inverseModelView;
    vec4 uiCursorWidget;
    vec4 uiCursorClip;
    uiCursorWidget[0] = prwiCursorX() / prwuiGetUIScaleFactor();
    uiCursorWidget[1] = prwiCursorY() / prwuiGetUIScaleFactor();
    uiCursorWidget[2] = 0.0f;
    uiCursorWidget[3] = 1.0f;
    glm_vec4_copy(uiCursorWidget, uiCursorClip);

    glm_mat4_inv(m_clipModelView, inverseModelView);
    glm_mat4_mulv(inverseModelView, uiCursorClip, uiCursorClip);

    char cursorInBounds = w->clipping ? uiCursorClip[0] > m_leftClip && uiCursorClip[1] > m_topClip && uiCursorClip[0] < m_rightClip && uiCursorClip[1] < m_bottomClip : 1;

    glm_mat4_inv(w->modelView, inverseModelView);
    glm_mat4_mulv(inverseModelView, uiCursorWidget, uiCursorWidget);

    char hovered = uiCursorWidget[0] >= w->x && uiCursorWidget[1] >= w->y && uiCursorWidget[0] < w->x + w->width && uiCursorWidget[1] < w->y + w->height;

    w->hovered = (w->zLevel == m_currentZLevel) && hovered && cursorInBounds;
}

static inline void i_lUpdate()
{
    for(int i = 0; i < m_listSize; i++)
    {
        if(m_widgetList[i].shouldRemove)
        {
            free(m_widgetList[i].ptr);
            if(i + 1 < m_listSize) memcpy(&m_widgetList[i], &m_widgetList[i + 1], m_listSize - (i + 1));
            m_listSize--;
            i--;
        }
    }
}

static struct Widget* i_lAdd(struct Widget widget)
{
    struct Widget* ptr = malloc(sizeof(struct Widget));

    if(ptr)
    {
        struct l_Element* newListBlock = m_widgetList;
        if(!m_widgetList) newListBlock = malloc(sizeof(struct l_Element) * m_listCapacity);
        else if((m_listSize + 1) > m_listCapacity)
        {
            newListBlock = realloc(m_widgetList, m_listCapacity * 2);
            if(newListBlock) m_listCapacity *= 2;
        }

        if(!newListBlock) 
        {
            free(ptr);
            return NULL;
        }

        m_widgetList = newListBlock;
        struct l_Element newElement;

        newElement.ptr = ptr;
        newElement.shouldRemove = 0;
        *ptr = widget;

        m_widgetList[m_listSize] = newElement;
        m_listSize++;
    }

    return ptr;
}

static inline void i_lClear()
{
    for(int i = 0; i < m_listSize; i++)
    {
        free(m_widgetList[i].ptr);
    }

    m_listSize = 0;
}

static inline void i_lRemove(struct Widget* widget)
{
    for(int i = 0; i < m_listSize; i++)
    {
        if(m_widgetList[i].ptr == widget)
        {
            m_widgetList[i].shouldRemove = 1;
            return;
        }
    }
}