#include "shaders.h"

#include <stdio.h>
#include <stdlib.h>

#ifndef EMSCRIPTEN
#include "gl3_shaders.h"
#else
#include "gles3_shaders.h"
#endif

#define U_PROJECTION_M_BIT 0
#define U_MODELVIEW_M_BIT 1
#define U_COLOR_BIT 2
#define U_TEXTURES_BIT 3

static int m_maxTexUnits = 16;
static int m_texUnits[16];
static char m_hasInit = 0;
static GLuint m_currentBoundProgram;
static uint32_t m_uniformUpdateFlags = 0xFFFFFFFF;
static mat4 m_projectionMatrix;
static mat4 m_modelViewMatrix;
static vec4 m_color;

static struct BasicShader
{
    GLuint programID;
    GLuint u_projectionMatrix;
    GLuint u_modelViewMatrix;
    GLuint u_color;
    GLuint u_textures;
} BasicShader;

static struct BasicShader* POS_SHADER = NULL;
static struct BasicShader* POS_UV_SHADER = NULL;
static struct BasicShader* POS_COLOR_SHADER = NULL;
static struct BasicShader* POS_UV_COLOR_TEXID_SHADER = NULL;

static inline void i_useBasicShader(struct BasicShader** shader, const char* vcode, const char* fcode);
static inline void i_setNthBit(uint32_t* flags, int bit);
static inline void i_clearNthBit(uint32_t* flags, int bit);
static inline int i_getNthBit(uint32_t* flags, int bit);

void prwsSetProjectionMatrix(mat4 matrix)
{
    glm_mat4_copy(matrix, m_projectionMatrix);
    i_setNthBit(&m_uniformUpdateFlags, U_PROJECTION_M_BIT);
}

void prwsSetModelViewMatrix(mat4 matrix)
{
    glm_mat4_copy(matrix, m_modelViewMatrix);
    i_setNthBit(&m_uniformUpdateFlags, U_MODELVIEW_M_BIT);
}

void prwsSetColorv(vec4 color)
{
    glm_vec4_copy(color, m_color);
    i_setNthBit(&m_uniformUpdateFlags, U_COLOR_BIT);
}

void prwsSetColor(float r, float g, float b, float a)
{
    m_color[0] = r;
    m_color[1] = g;
    m_color[2] = b;
    m_color[3] = a;
    i_setNthBit(&m_uniformUpdateFlags, U_COLOR_BIT);
}

vec4* prwsGetProjectionMatrix()
{
    return (vec4*) m_projectionMatrix;
}

vec4* prwsGetModelViewMatrix()
{
    return (vec4*) m_modelViewMatrix;
}

float* prwsGetColor()
{
    return m_color;
}

void prws_POS_shader()
{
    i_useBasicShader(&POS_SHADER, POS_SHADER_vcode, POS_SHADER_fcode);
}

void prws_POS_UV_shader()
{
    i_useBasicShader(&POS_UV_SHADER, POS_UV_SHADER_vcode, POS_UV_SHADER_fcode);
}

void prws_POS_COLOR_shader()
{
    i_useBasicShader(&POS_COLOR_SHADER, POS_COLOR_SHADER_vcode, POS_COLOR_SHADER_fcode);
}

void prws_POS_UV_COLOR_TEXID_shader()
{
    i_useBasicShader(&POS_UV_COLOR_TEXID_SHADER, POS_UV_COLOR_TEXID_SHADER_vcode, POS_UV_COLOR_TEXID_SHADER_fcode);
}

static void i_printShaderLog(GLuint shader, const char* shaderType)
{
    char log[1024] = {0};
    glGetShaderInfoLog(shader, sizeof(log), NULL, log);

    printf("[%s Shader][Error]:\n%s\n\n", shaderType ? shaderType : "GL", log);
}

static void i_printProgramLog(GLuint program, const char* programName)
{
    char log[1024] = {0};
    glGetProgramInfoLog(program, sizeof(log), NULL, log);

    printf("[%s Program][Error]:\n%s\n\n", programName ? programName : "GL", log);
}

struct BasicShader i_createBasicShader(const char* vecCode, const char* fragCode)
{
    struct BasicShader shader;
    shader.programID = glCreateProgram();
    GLuint vecShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vecShader, 1, &vecCode, NULL);
    glCompileShader(vecShader);

    GLint success;
    glGetShaderiv(vecShader, GL_COMPILE_STATUS, &success);
    if(success == GL_FALSE)
    {
        i_printShaderLog(vecShader, "Vertex");
        goto link;
    }

    glShaderSource(fragShader, 1, &fragCode, NULL);
    glCompileShader(fragShader);

    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if(success == GL_FALSE)
        i_printShaderLog(fragShader, "Fragment");

    link:
    if(success)
    {
        glAttachShader(shader.programID, vecShader);
        glAttachShader(shader.programID, fragShader);
        glLinkProgram(shader.programID);

        glGetProgramiv(shader.programID, GL_LINK_STATUS, &success);
        if(success == GL_FALSE)
            i_printProgramLog(shader.programID, "GL Program");

        shader.u_projectionMatrix = glGetUniformLocation(shader.programID, "u_projectionMatrix");
        shader.u_modelViewMatrix = glGetUniformLocation(shader.programID, "u_modelViewMatrix");
        shader.u_color = glGetUniformLocation(shader.programID, "u_color");
        shader.u_textures = glGetUniformLocation(shader.programID, "u_textures");
    }
    else
    {
        glDeleteProgram(shader.programID);
        shader.programID = 0;
    }

    glDeleteShader(vecShader);
    glDeleteShader(fragShader);

    return shader;
}

#define u_Assert(t, b) if(i_getNthBit(&m_uniformUpdateFlags, b)) { t; i_clearNthBit(&m_uniformUpdateFlags, b); }

static inline void i_useBasicShader(struct BasicShader** shaderPtrR, const char* vcode, const char* fcode)
{
    if(!(*shaderPtrR))
    {
        *shaderPtrR = malloc(sizeof(struct BasicShader));
        **shaderPtrR = i_createBasicShader(vcode, fcode);

        if(!m_hasInit)
        {
            glm_mat4_identity(m_projectionMatrix);
            glm_mat4_identity(m_modelViewMatrix);
            prwsSetColor(1.0f, 1.0f, 1.0f, 1.0f);
            m_hasInit = 1;

            glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m_maxTexUnits);
            m_maxTexUnits = m_maxTexUnits <= 16 ? m_maxTexUnits : 16;

            for(int i = 0; i < m_maxTexUnits; i++) m_texUnits[i] = i;
        }
        return;
    }

    struct BasicShader* shader = *shaderPtrR;

    if(!shader->programID) return;

    if(m_currentBoundProgram != shader->programID)
    {
        glUseProgram(shader->programID);
        m_currentBoundProgram = shader->programID;
        m_uniformUpdateFlags = 0xFFFFFFF;
    }

    u_Assert(glUniformMatrix4fv(shader->u_projectionMatrix, 1, GL_FALSE, (float*) m_projectionMatrix), U_PROJECTION_M_BIT)
    u_Assert(glUniformMatrix4fv(shader->u_modelViewMatrix, 1, GL_FALSE, (float*) m_modelViewMatrix), U_MODELVIEW_M_BIT)
    u_Assert(glUniform4fv(shader->u_color, 1, m_color), U_COLOR_BIT)
    u_Assert(glUniform1iv(shader->u_textures, m_maxTexUnits, m_texUnits);, U_TEXTURES_BIT);
}

static inline void i_setNthBit(uint32_t* flags, int bit)
{
    uint32_t set = ((uint32_t) 1) << bit;
    *flags |= set;
}

static inline void i_clearNthBit(uint32_t* flags, int bit)
{
    uint32_t set = ~(((uint32_t) 1) << bit);
    *flags &= set;
}

static inline int i_getNthBit(uint32_t* flags, int bit)
{
    uint32_t value = ((uint32_t) 1) << bit;
    value &= *flags;
    value = value >> bit;

    return value;
}