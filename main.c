#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

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

void onAction(PRWwidget* widget)
{
    printf("widget clicked!\n");
}

void loadTexture(const char* url, GLuint* texId)
{
    PRWfetcher* imageFetcher = prwfFetchURL(url);
    prwfFetchWait(imageFetcher);

    int x, y, channels, len;
    stbi_set_flip_vertically_on_load(1);
    const char* imgf = prwfFetchString(imageFetcher, &len);
    stbi_uc* image = stbi_load_from_memory(imgf, len, &x, &y, &channels, STBI_rgb_alpha);

    if(!image)
    {
        printf("failed to load texture! %s\n", stbi_failure_reason());
        exit(1);
    }

    glGenTextures(1, texId);

    glBindTexture(GL_TEXTURE_2D, *texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(image);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Parkour Recorder Website", NULL, NULL);

    prwiRegisterWindow(window);
    prwiSetActiveWindow(window);
    prwuiSetWindow(window);

    glfwSwapInterval(1);

    setupCube();
    prwmLoad("res/cube_map.bin");
    backgroundMesh = prwmMeshGet("cube_map");
    prwInitMenuScreen();
    prwaInitSmoother(&smoother);
    smoother.speed = 5;

    double lastTime = glfwGetTime();

    srand(time(0));
    camLook[1] = ((double)rand() / RAND_MAX) * 360;

    //loadTexture("https://elmfer.com/parkour_recorder/parkour_recorder_logo.png", &texID);

    while(!glfwWindowShouldClose(window))
    {
        handleKeyEvents(window);
        handleMouseEvents(window);
        handleCursorMovement(window);

        float wX = prwuiGetWindowWidth(), wY = prwuiGetWindowHeight();
        glViewport(0, 0, wX, wY);

        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

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
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        prwuiGenQuad(0, 0, 250, 250, -1, 1);
        prwuiRenderBatch();

        prwwTickWidgets();
        prwiPollInputs();
        clearGLErrors();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    printf("Hellow Worlds!\n");

    return 0;
}

void setupCube()
{
    GLuint glBuffer;
    GLuint glElemBuffer;

    glGenBuffers(1, &glBuffer);
    glGenBuffers(1, &glElemBuffer);
    glGenVertexArrays(1, &quadVAO);

    struct Vertex
    {
        vec3 pos;
        vec2 uv;
    } Vertex;

    struct Vertex verticies[] =
    {
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}},
    };

    int indicies[] =
    {
        0, 1, 2,
        0, 2, 3,
    };

    glBindVertexArray(quadVAO);
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glElemBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicies), indicies, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, glBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verticies), verticies, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(struct Vertex), 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(Vertex), (void *) sizeof(vec3));
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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