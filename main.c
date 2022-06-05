#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#ifndef EMSCRIPTEN
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#include <emscripten.h>
#endif

#include <cglm/cglm.h>
#include <stb_image.h>

#include "shaders.h"
#include "input.h"
#include "ui_render.h"
#include "widget.h"
#include "screen_renderer.h"
#include "mesh.h"
#include "animation.h"
#include "https_fetcher.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

GLuint texID = 0;
GLuint quadVAO = 0;
PRWmesh* backgroundMesh = NULL;
PRWsmoother smoother;

char mouseLocked = 0;
vec3 camPos;
vec3 camLook;

mat4 projectionMatrix;
mat4 modelViewMatrix;

void loadShaders();
void setupCube();
void clearGLErrors();

void handleKeyEvents(GLFWwindow* window);
void handleMouseEvents(GLFWwindow* window);
void handleCursorMovement(GLFWwindow* window);

static GLFWwindow* window;
static double lastTime = 0.0;
static size_t numFramesPassed = 0;

char PRW_ON_MOBILE = 0;

void loadBackground()
{   
    prwmLoad(PRW_ON_MOBILE ? "res/cube_map_512.obj" : "res/cube_map.obj");
    backgroundMesh = prwmMeshGet("cube_map");

    prwvfVTXFMT bgVtxFmt;
    bgVtxFmt[0] = 2;

    bgVtxFmt[1] = PRWVF_ATTRB_USAGE_POS
                | PRWVF_ATTRB_TYPE_FLOAT
                | PRWVF_ATTRB_SIZE(3)
                | PRWVF_ATTRB_NORMALIZED_FALSE;

    bgVtxFmt[2] = PRWVF_ATTRB_USAGE_UV
                | PRWVF_ATTRB_TYPE_FLOAT
                | PRWVF_ATTRB_SIZE(2)
                | PRWVF_ATTRB_NORMALIZED_FALSE;

    prwmMakeRenderable(backgroundMesh, bgVtxFmt);
}

static void drawLoadingScreen()
{
    float uiWidth = prwuiGetUIwidth();
    float uiHeight = prwuiGetUIheight();

    //Make sure ui shader is absolutely ready
    prws_POS_UV_COLOR_TEXID_shader();
    glFinish();

    //Display loading screen
    prwuiSetupUIrendering();
    prwuiGenGradientQuad(PRWUI_TO_BOTTOM, 0, 0, uiWidth, uiHeight, -11776948, 0, 0);
    prwuiPushStack();
    {
        prwuiTranslate(uiWidth / 2, uiHeight / 2);
        prwuiScale(3, 3);
        prwuiGenString(PRWUI_CENTER, "Loading Assets...", 0, 0, -9276814);
        prwuiGenString(PRWUI_CENTER, "Loading Assets...", 0, -1, -1);
    }
    prwuiPopStack();
    prwuiRenderBatch();
}

static void mainLoop()
{
    handleKeyEvents(window);
    handleMouseEvents(window);
    handleCursorMovement(window);

    float wX = prwuiGetWindowWidth(), wY = prwuiGetWindowHeight();
    glViewport(0, 0, wX, wY);

    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if(numFramesPassed < 3)
    {
        if(numFramesPassed == 1)
        {
            loadBackground();
        }

        drawLoadingScreen();

        glfwSwapBuffers(window);
        glfwPollEvents();

        numFramesPassed++;
        return;
    }

    double time = glfwGetTime();
    camLook[0] = prwaSmootherValue(&smoother);
    camLook[1] += (time - lastTime) * 4;
    lastTime = time;

    glm_perspective(glm_rad(60), wX / wY, 0.1, 100, projectionMatrix);
    glm_mat4_identity(modelViewMatrix);
    vec3 axis = {1.0f, 0.0f, 0.0f};
    glm_rotate(modelViewMatrix, glm_rad(camLook[0]), axis);
    axis[0] = 0.0f; axis[1] = 1.0f;
    glm_rotate(modelViewMatrix, glm_rad(camLook[1]), axis);
    glm_translate(modelViewMatrix, camPos);

    glEnable(GL_DEPTH_TEST);
    prwsSetProjectionMatrix(projectionMatrix);
    prwsSetModelViewMatrix(modelViewMatrix);
    prws_POS_UV_shader();
    prwmMeshRenderv(backgroundMesh);

    prwRenderMenuScreen();
    prwuiRenderBatch();

    prwwTickWidgets();
    prwiPollInputs();
    clearGLErrors();
    glfwSwapBuffers(window);
    glfwPollEvents();

    numFramesPassed++;
}

int main(int argc, char** argv)
{
    int initWidth = 1280, initHeight = 720;
    if(argc == 3)
    {
        initWidth = atoi(argv[1]);
        initHeight = atoi(argv[2]);
        if(initWidth < 128 || 8192 < initWidth) initWidth = 1280;
        if(initHeight < 128 || 8192 < initHeight) initHeight = 720;
    }
    for(int a = 1; a < argc; a++)
    {
        if(strcmp(argv[a], "-on-mobile") == 0) PRW_ON_MOBILE = 1;
    }
    printf("[Main][Info]: Setting initial window size to (%d, %d)\n", initWidth, initHeight);
    if(PRW_ON_MOBILE) printf("[Main][Info]: Program is running for mobile\n");

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);

    window = glfwCreateWindow(initWidth, initHeight, "Parkour Recorder Website", NULL, NULL);

    prwiRegisterWindow(window);
    prwiSetActiveWindow(window);
    prwuiSetWindow(window);

    glfwSwapInterval(1);

    prwInitMenuScreen();
    prwaInitSmoother(&smoother);
    smoother.speed = 5;

    lastTime = glfwGetTime();

    srand(time(0));
    camLook[1] = ((double)rand() / RAND_MAX) * 360;

#ifndef EMSCRIPTEN
    while(!glfwWindowShouldClose(window)) mainLoop();
#else
    emscripten_set_main_loop(mainLoop, 0, true);
#endif

    glfwTerminate();
    printf("Hellow Worlds!\n");

    return 0;
}

void clearGLErrors()
{
    GLenum error;
    do
    {
        error = glGetError();

        if(error != GL_NO_ERROR)
            printf("[OpenGL][Error]: ID: %d\n", error);
    }
    while (error != GL_NO_ERROR);
    
}

void handleKeyEvents(GLFWwindow* window)
{
    if(prwiKeyJustPressed(GLFW_KEY_ESCAPE))
    {
        if(mouseLocked)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            mouseLocked = 0;
        }
        else
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    if(!mouseLocked) return;

    float speed = 0.05f;
    vec3 strafe; glm_vec3_zero(strafe);
    if(prwiIsKeyPressed(GLFW_KEY_SPACE)) camPos[1] -= speed;
    if(prwiIsKeyPressed(GLFW_KEY_LEFT_SHIFT)) camPos[1] += speed;
    if(prwiIsKeyPressed(GLFW_KEY_W)) strafe[2] += speed;
    if(prwiIsKeyPressed(GLFW_KEY_S)) strafe[2] -= speed;
    if(prwiIsKeyPressed(GLFW_KEY_A)) strafe[0] += speed;
    if(prwiIsKeyPressed(GLFW_KEY_D)) strafe[0] -= speed;
    if(prwiKeyJustPressed(GLFW_KEY_R)) glm_vec3_zero(camPos);

    vec3 axis = {0.0f, 1.0f, 0.0f};
    glm_vec3_rotate(strafe, -glm_rad(camLook[1]), axis);

    glm_vec3_add(strafe, camPos, camPos);
}

void handleMouseEvents(GLFWwindow* window)
{
    if(prwiMButtonJustPressed(GLFW_MOUSE_BUTTON_1) && !mouseLocked)
    {
        //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        //mouseLocked = 1;
    }
}

void handleCursorMovement(GLFWwindow* window)
{
    smoother.grabbingTo = 5 * ((prwiCursorY() - prwuiGetWindowHeight() / 2) / (prwuiGetWindowHeight() / 2));
    if(mouseLocked)
    {
        camLook[0] += prwiCursorYDelta() * 0.15f;
        camLook[1] += prwiCursorXDelta() * 0.15f;
    }
}