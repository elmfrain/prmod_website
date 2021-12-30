#include "input.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <cglm/vec2.h>

#define CONTEXT_LIST_MAX_SIZE 16
#define INT_KEYFLAGS_SIZE 88

static struct InputContext
{
    GLFWwindow* window;

    uint32_t keyPressedStates[INT_KEYFLAGS_SIZE];
    uint32_t keyDownStates[INT_KEYFLAGS_SIZE];
    uint32_t keyReleasedStates[INT_KEYFLAGS_SIZE];
    uint32_t keyRepeatingStates[INT_KEYFLAGS_SIZE];

    uint32_t mousePressedStates;
    uint32_t mouseDownStates;
    uint32_t mouseReleasedStates;

    vec2 prevCursorPos;
    vec2 cursorPos;
} InputContext;

static size_t m_contextListSize = 0;
static struct InputContext* m_contexts = NULL;
static struct InputContext* m_activeContext;

//Callbacks
static void onKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods);
static void onMouseEvent(GLFWwindow* window, int button, int action, int mods);
static void onCursorMove(GLFWwindow* window, double cursorX, double cursorY);

static struct InputContext* i_getContextFromWindow(GLFWwindow* window);
static int i_getNthBit(uint32_t* flagBuffer, int bit);

void prwiRegisterWindow(GLFWwindow* window)
{
    if(!m_contexts)
        m_contexts = malloc(sizeof(struct InputContext) * CONTEXT_LIST_MAX_SIZE);

    if(m_contextListSize == CONTEXT_LIST_MAX_SIZE)
    {
        printf("[Input][Error]: Cannot register more than %d windows!", CONTEXT_LIST_MAX_SIZE);
        return;
    }

    memset(&m_contexts[m_contextListSize], 0, sizeof(struct InputContext));

    m_contexts[m_contextListSize].window = window;
    m_contextListSize++;

    glfwSetKeyCallback(window, onKeyEvent);
    glfwSetMouseButtonCallback(window, onMouseEvent);
    glfwSetCursorPosCallback(window, onCursorMove);
}

void prwiSetActiveWindow(GLFWwindow* window)
{
    if(m_activeContext && window == m_activeContext->window) return;

    struct InputContext* context = i_getContextFromWindow(window);

    if(!context) return;

    m_activeContext = context;
}

void prwiPollInputs()
{
    for(int i = 0; i < m_contextListSize; i++)
    {
        memset(m_contexts[i].keyPressedStates, 0, INT_KEYFLAGS_SIZE * 4);
        memset(m_contexts[i].keyReleasedStates, 0, INT_KEYFLAGS_SIZE * 4);
        memset(m_contexts[i].keyRepeatingStates, 0, INT_KEYFLAGS_SIZE * 4);
        memset(&m_contexts[i].mousePressedStates, 0, sizeof(uint32_t));
        memset(&m_contexts[i].mouseReleasedStates, 0, sizeof(uint32_t));

        glm_vec2_copy(m_contexts[i].cursorPos, m_contexts[i].prevCursorPos);
    }
}

int prwiKeyJustPressed(int key)
{
    if(!m_activeContext) return 0;
    return i_getNthBit(m_activeContext->keyPressedStates, key);
}

int prwiIsKeyPressed(int key)
{
    if(!m_activeContext) return 0;
    return i_getNthBit(m_activeContext->keyDownStates, key);
}

int prwiKeyJustReleased(int key)
{
    if(!m_activeContext) return 0;
    return i_getNthBit(m_activeContext->keyReleasedStates, key);
}

int prwiIsKeyRepeating(int key)
{
    if(!m_activeContext) return 0;
    return i_getNthBit(m_activeContext->keyRepeatingStates, key);
}

int prwiIsMButtonPressed(int button)
{
    if(!m_activeContext) return 0;
    return i_getNthBit(&m_activeContext->mouseDownStates, button);
}

int prwiMButtonJustPressed(int button)
{
    if(!m_activeContext) return 0;
    return i_getNthBit(&m_activeContext->mousePressedStates, button);
}

int prwiMButtonJustReleased(int button)
{
    if(!m_activeContext) return 0;
    return i_getNthBit(&m_activeContext->mouseReleasedStates, button);
}

float prwiCursorX()
{
    if(!m_activeContext) return 0;
    return m_activeContext->cursorPos[0];
}

float prwiCursorY()
{
    if(!m_activeContext) return 0;
    return m_activeContext->cursorPos[1];
}

float prwiCursorXDelta()
{
    if(!m_activeContext) return 0;
    return m_activeContext->cursorPos[0] - m_activeContext->prevCursorPos[0];
}

float prwiCursorYDelta()
{
    if(!m_activeContext) return 0;
    return m_activeContext->cursorPos[1] - m_activeContext->prevCursorPos[1];
}

struct InputContext* i_getContextFromWindow(GLFWwindow* window)
{
    for(int i = 0; i < m_contextListSize; i++)
    {
        if(m_contexts[i].window == window)
            return &m_contexts[i];
    }

    return 0;
}

static inline void i_setNthBit(uint32_t* flagBuffer, int bit)
{
    uint32_t* flagChunk = flagBuffer + (bit / sizeof(int32_t));
    int bitPos = bit % 32;

    uint32_t set = ((uint32_t) 1) << bitPos;
    *flagChunk |= set;
}

static inline void i_clearNthBit(uint32_t* flagBuffer, int bit)
{
    uint32_t* flagChunk = flagBuffer + (bit / sizeof(int32_t));
    int bitPos = bit % 32;

    uint32_t set = ~(((uint32_t) 1) << bitPos);
    *flagChunk &= set;
}

static inline int i_getNthBit(uint32_t* flagBuffer, int bit)
{
    uint32_t* flagChunk = flagBuffer + (bit / sizeof(int32_t));
    int bitPos = bit % 32;

    uint32_t value = ((uint32_t) 1) << bitPos;
    value &= *flagChunk;
    value = value >> bitPos;

    return value;
}

static void onKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    struct InputContext* context = i_getContextFromWindow(window);
    if(!context) return;

    if(action == GLFW_PRESS)
    {
        i_setNthBit(context->keyPressedStates, key);
        i_setNthBit(context->keyDownStates, key);
    }
    else if(action == GLFW_RELEASE)
    {
        i_clearNthBit(context->keyDownStates, key);
        i_setNthBit(context->keyReleasedStates, key);
    }
    else if(action == GLFW_REPEAT)
    {
        i_setNthBit(context->keyReleasedStates, key);
    }
}

static void onMouseEvent(GLFWwindow* window, int button, int action, int mods)
{
    struct InputContext* context = i_getContextFromWindow(window);
    if(!context) return;

    if(action == GLFW_PRESS)
    {
        i_setNthBit(&context->mousePressedStates, button);
        i_setNthBit(&context->mouseDownStates, button);
    }
    else if(action == GLFW_RELEASE)
    {
        i_clearNthBit(&context->mouseDownStates, button);
        i_setNthBit(&context->mouseReleasedStates, button);
    }
}

static void onCursorMove(GLFWwindow* window, double cursorX, double cursorY)
{
    struct InputContext* context = i_getContextFromWindow(window);
    if(!context) return;
    
    context->cursorPos[0] = cursorX;
    context->cursorPos[1] = cursorY;
}