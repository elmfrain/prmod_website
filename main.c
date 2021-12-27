#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <cglm/cglm.h>

#include "shaders.h"
#include "input.h"

#include <stdio.h>

GLuint quadVAO = 0;

char mouseLocked = 0;
vec3 camPos;
vec3 camLook;

vec2 windowSize;

mat4 projectionMatrix;
mat4 modelViewMatrix;

void printShaderLog(GLuint shader, const char* shaderType);
void printProgramLog(GLuint program, const char* programName);
void loadShaders();
void setupQuad();
void clearGLErrors();

void handleKeyEvents(GLFWwindow* window);
void handleMouseEvents(GLFWwindow* window);
void handleCursorMovement(GLFWwindow* window);
void onResize(GLFWwindow* window, int width, int height);

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Parkour Recorder Website", NULL, NULL);

    prwiRegisterWindow(window);
    prwiSetActiveWindow(window);

    int width, height;
    glfwGetWindowSize(window, &width, &height);
    windowSize[0] = width; windowSize[1] = height;

    glfwSetWindowSizeCallback(window, onResize);

    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    glfwSwapInterval(1);

    setupQuad();

    while(!glfwWindowShouldClose(window))
    {
        handleKeyEvents(window);
        handleMouseEvents(window);
        handleCursorMovement(window);

        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glm_perspective(120, windowSize[0] / windowSize[1], 0.1, 100, projectionMatrix);
        glm_mat4_identity(modelViewMatrix);
        vec3 axis = {1.0f, 0.0f, 0.0f};
        glm_rotate(modelViewMatrix, glm_rad(camLook[0]), axis);
        axis[0] = 0.0f; axis[1] = 1.0f;
        glm_rotate(modelViewMatrix, glm_rad(camLook[1]), axis);
        glm_translate(modelViewMatrix, camPos);

        prwsSetProjectionMatrix(projectionMatrix);
        prwsSetModelViewMatrix(modelViewMatrix);
        prwsUsePOSshader();

        glBindVertexArray(quadVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);

        prwiPollInputs();
        clearGLErrors();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    printf("Hellow Worlds!\n");

    return 0;
}

void setupQuad()
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
        {{-0.5f, -0.5f, -4.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -4.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -4.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -4.0f}, {0.0f, 1.0f}}
    };

    int indicies[] =
    {
        0, 1, 2,
        0, 2, 3
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
    vec3 strafe; glm_vec2_zero(strafe);
    if(prwiIsKeyPressed(GLFW_KEY_SPACE)) camPos[1] -= speed;
    if(prwiIsKeyPressed(GLFW_KEY_LEFT_SHIFT)) camPos[1] += speed;
    if(prwiIsKeyPressed(GLFW_KEY_W)) strafe[2] += speed;
    if(prwiIsKeyPressed(GLFW_KEY_S)) strafe[2] -= speed;
    if(prwiIsKeyPressed(GLFW_KEY_A)) strafe[0] += speed;
    if(prwiIsKeyPressed(GLFW_KEY_D)) strafe[0] -= speed;

    vec3 axis = {0.0f, 1.0f, 0.0f};
    glm_vec3_rotate(strafe, -glm_rad(camLook[1]), axis);

    glm_vec3_add(strafe, camPos, camPos);
}

void handleMouseEvents(GLFWwindow* window)
{
    if(prwiMButtonJustPressed(GLFW_MOUSE_BUTTON_1) && !mouseLocked)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseLocked = 1;
    }
}

void handleCursorMovement(GLFWwindow* window)
{
    if(mouseLocked)
    {
        camLook[0] += prwiCursorYDelta() * 0.15f;
        camLook[1] += prwiCursorXDelta() * 0.15f;
    }
}

void onResize(GLFWwindow* window, int width, int height)
{
    windowSize[0] = width;
    windowSize[1] = height;

    glViewport(0, 0, width, height);
}