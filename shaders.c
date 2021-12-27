#include "shaders.h"

#include <stdio.h>

static const char* POS_SHADER_vcode =
"#version 150 core\n"
"in vec3 i_pos;\n"
"uniform mat4 u_projectionMatrix;\n"
"uniform mat4 u_modelViewMatrix;\n"
"\n"
"void main()\n"
"{\n"
"   gl_Position = u_projectionMatrix * u_modelViewMatrix * vec4(i_pos, 1.0);\n"
"}\n"
;

static const char* POS_SHADER_fcode =
"#version 150 core\n"
"out vec4 o_fragColor;\n"
"uniform vec4 u_color;\n"
"\n"
"void main()\n"
"{\n"
"   o_fragColor = u_color;\n"
"}\n"
;

static const char* POS_UV_SHADER_vcode =
"#version 150 core\n"
"in vec3 i_pos;\n"
"uniform mat4 u_projectionMatrix;\n"
"uniform mat4 u_modelViewMatrix;\n"
"\n"
"void main()\n"
"{\n"
"   gl_Position = u_projectionMatrix * u_modelViewMatrix * vec4(i_pos, 1.0);\n"
"}\n"
;

static const char* POS_UV_SHADER_fcode =
"#version 150 core\n"
"out vec4 o_fragColor;\n"
"\n"
"void main()\n"
"{\n"
"   o_fragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
"}\n"
;

static char m_hasInit = 0;
static mat4 m_projectionMatrix;
static mat4 m_modelViewMatrix;
static vec4 m_color;

static struct BasicShader
{
    GLuint programID;
    GLuint u_projectionMatrix;
    GLuint u_modelViewMatrix;
    GLuint u_color;
} BasicShader;

static struct BasicShader* POS_SHADER = NULL;

static inline void i_useBasicShader(struct BasicShader** shader, const char* vcode, const char* fcode);

void prwsSetProjectionMatrix(mat4 matrix)
{
    glm_mat4_copy(matrix, m_projectionMatrix);
}

void prwsSetModelViewMatrix(mat4 matrix)
{
    glm_mat4_copy(matrix, m_modelViewMatrix);
}

void prwsSetColorv(vec4 color)
{
    glm_vec4_copy(color, m_color);
}

void prwsSetColor(float r, float g, float b, float a)
{
    m_color[0] = r;
    m_color[1] = g;
    m_color[2] = b;
    m_color[3] = a;
}

float* prwsGetProjectionMatrix()
{
    return (float*) m_projectionMatrix;
}

float* prwsGetModelViewMatrix()
{
    return (float*) m_modelViewMatrix;
}

float* prwsGetColor()
{
    return m_color;
}

void prwsUsePOSshader()
{
    i_useBasicShader(&POS_SHADER, POS_SHADER_vcode, POS_SHADER_fcode);
}

void prwsUsePOS_UVshader();

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

struct BasicShader i_createShader(const char* vecCode, const char* fragCode)
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
            i_printProgramLog(shader.programID, "Pos Shader");

        shader.u_projectionMatrix = glGetUniformLocation(shader.programID, "u_projectionMatrix");
        shader.u_modelViewMatrix = glGetUniformLocation(shader.programID, "u_modelViewMatrix");
        shader.u_color = glGetUniformLocation(shader.programID, "u_color");
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

static inline void i_useBasicShader(struct BasicShader** shaderPtrR, const char* vcode, const char* fcode)
{
    if(!(*shaderPtrR))
    {
        *shaderPtrR = malloc(sizeof(struct BasicShader));
        **shaderPtrR = i_createShader(vcode, fcode);

        if(!m_hasInit)
        {
            glm_mat4_identity(m_projectionMatrix);
            glm_mat4_identity(m_modelViewMatrix);
            prwsSetColor(1.0f, 1.0f, 1.0f, 1.0f);
            m_hasInit = 1;
        }
        return;
    }

    struct BasicShader* shader = *shaderPtrR;

    if(!shader->programID) return;

    glUseProgram(shader->programID);
    glUniformMatrix4fv(shader->u_projectionMatrix, 1, GL_FALSE, (float*) m_projectionMatrix);
    glUniformMatrix4fv(shader->u_modelViewMatrix, 1, GL_FALSE, (float*) m_modelViewMatrix);
    glUniform4fv(shader->u_color, 1, m_color);
}
