#include "mesh_builder.h"

#include <stdarg.h>
#include <string.h>

#include <cglm/mat3.h>

//-----------------------VertexFormat Class--------------------------

void prwvfApply(prwvfVTXFMT vtxFmt)
{
    uint32_t numAttributes = vtxFmt[0];
    uint32_t stride = prwvfVertexNumBytes(vtxFmt);

    size_t pointer = 0;
    for(uint32_t i = 1; i <= numAttributes; i++)
    {
        uint32_t numBytes = prwvfaNumBytes(vtxFmt[i]);
        uint32_t glType = prwvfaGetGLType(vtxFmt[i]);
        uint32_t size = prwvfaGetSize(vtxFmt[i]);
        bool normalized = prwvfaIsNormalized(vtxFmt[i]);

        glEnableVertexAttribArray(i - 1);
        glVertexAttribPointer(i - 1, size, glType, normalized, stride, (void*) pointer);
        pointer += numBytes;
    }
}

void prwvfUnapply(prwvfVTXFMT vtxFmt)
{
    uint32_t numAttributes = vtxFmt[0];

    for(uint32_t i = 1; i <= numAttributes; i++)
    {
        glDisableVertexAttribArray(i - 1);
    }
}

uint32_t prwvfVertexNumBytes(prwvfVTXFMT vtxFmt)
{
    uint32_t numAttributes = vtxFmt[0];
    uint32_t numBytes = 0;

    for(uint32_t i = 1; i <= numAttributes; i++)
    {
        numBytes += prwvfaNumBytes(vtxFmt[i]);
    }

    return numBytes;
}

uint32_t prwvfaGetUsage(prwvfVTXATTRB attrib)
{
    return attrib & PRWVF_ATTRB_USAGE_MASK;
}

uint32_t prwvfaGetSize(prwvfVTXATTRB attrib)
{
    return (attrib & PRWVF_ATTRB_SIZE_MASK) >> 8;
}

uint32_t prwvfaGetType(prwvfVTXATTRB attrib)
{
    return attrib & PRWVF_ATTRB_TYPE_MASK;
}

uint32_t prwvfaGetGLType(prwvfVTXATTRB attrib)
{
    uint32_t type = attrib & PRWVF_ATTRB_TYPE_MASK;

    switch (type)
    {
    case PRWVF_ATTRB_TYPE_FLOAT:
        return GL_FLOAT;
    case PRWVF_ATTRB_TYPE_INT:
        return GL_INT;
    case PRWVF_ATTRB_TYPE_UINT:
        return GL_UNSIGNED_INT;
    case PRWVF_ATTRB_TYPE_SHORT:
        return GL_SHORT;
    case PRWVF_ATTRB_TYPE_USHORT:
        return GL_UNSIGNED_SHORT;
    case PRWVF_ATTRB_TYPE_BYTE:
        return GL_BYTE;
    case PRWVF_ATTRB_TYPE_UBYTE:
        return GL_UNSIGNED_BYTE;
    default:
        return GL_FLOAT;
    }
}

uint32_t prwvfaNumBytes(prwvfVTXATTRB attrib)
{
    uint32_t size = (attrib & PRWVF_ATTRB_SIZE_MASK) >> 8;
    uint32_t typeSize = (attrib & PRWVF_ATTRB_TYPE_SIZE_MASK) >> 20;

    return size * typeSize;
}

bool prwvfaIsNormalized(prwvfVTXATTRB attrib)
{
    return (bool) (attrib & PRWVF_ATTRB_NORMALIZED_MASK);
}

//-----------------------MeshBuilder Class---------------------------

#ifndef PRWMB_MODELVIEW_STACK_SIZE
#define PRWMB_MODELVIEW_STACK_SIZE 128
#endif

#ifndef PRWMB_BUFFER_STARTING_SIZE
#define PRWMB_BUFFER_STARTING_SIZE 512
#endif

typedef struct MeshBuilder
{
    PRWmeshBuilder builder;

    prwvfVTXFMT m_vertexFormat;

    //Modelview variables
    mat4 m_modelViewStack[PRWMB_MODELVIEW_STACK_SIZE];
    mat4* m_currentModelView;
    size_t m_stackIndex;
    mat3 m_modelView3x3;

    //OpenGL varaibles
    GLuint m_glVBO;
    GLuint m_glEBO;
    GLuint m_glVAO;
    size_t m_glVertexBufferSize;
    size_t m_glElementBufferSize;

    //Book Keeping
    size_t m_numVerticies;
    size_t m_numIndicies;
    size_t m_vertexDataPos;
    size_t m_indexDataPos;

    //Storage
    size_t m_vertexDataBufferCapacity;
    uint8_t* m_vertexDataBuffer;
    size_t m_indexDataBufferCapacity;
    uint32_t* m_indexDataBuffer;
} MeshBuilder;

#define getMeshBuilder MeshBuilder* b = (MeshBuilder*) builder

static void i_mbPushVertexData(MeshBuilder* b, size_t size, void* data);
static void i_mbPushIndexData(MeshBuilder* b, size_t size, void* data);
static void i_mbMat4Get3x3(MeshBuilder* b);

PRWmeshBuilder* prwmbGenBuilder(prwvfVTXFMT vertexFormat)
{
    MeshBuilder* builder = malloc(sizeof(MeshBuilder));

    float defaultNormal[] = { 0.0f, 1.0f, 0.0f };
    float defaultUV[] = { 0.0f, 0.0f };
    float defaultColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    memcpy(builder->builder.defaultNormal, defaultNormal, sizeof(defaultNormal));
    memcpy(builder->builder.defualtUV, defaultUV, sizeof(defaultUV));
    memcpy(builder->builder.defaultColor, defaultColor, sizeof(defaultColor));

    prwmbResetMatrixStack((PRWmeshBuilder*) builder);

    builder->m_numVerticies = 0;
    builder->m_numIndicies = 0;
    builder->m_vertexDataPos = 0;
    builder->m_indexDataPos = 0;

    builder->m_vertexDataBufferCapacity = PRWMB_BUFFER_STARTING_SIZE;
    builder->m_vertexDataBuffer = malloc(PRWMB_BUFFER_STARTING_SIZE);
    builder->m_indexDataBufferCapacity = PRWMB_BUFFER_STARTING_SIZE;
    builder->m_indexDataBuffer = malloc(PRWMB_BUFFER_STARTING_SIZE);

    memcpy(builder->m_vertexFormat, vertexFormat, sizeof(prwvfVTXFMT));

    glGenBuffers(1, &builder->m_glVBO);
    glGenBuffers(1, &builder->m_glEBO);
    glGenVertexArrays(1, &builder->m_glVAO);

    builder->m_glVertexBufferSize = PRWMB_BUFFER_STARTING_SIZE;
    builder->m_glElementBufferSize = PRWMB_BUFFER_STARTING_SIZE;

    glBindVertexArray(builder->m_glVAO);
    {
        glBindBuffer(GL_ARRAY_BUFFER, builder->m_glVBO);
        glBufferData(GL_ARRAY_BUFFER, builder->m_glVertexBufferSize, NULL, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, builder->m_glEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, builder->m_glElementBufferSize, NULL, GL_DYNAMIC_DRAW);

        prwvfApply(builder->m_vertexFormat);

    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return (PRWmeshBuilder*) builder;
}

void prwmbDeleteBuilder(PRWmeshBuilder* builder)
{
    getMeshBuilder;

    free(b->m_vertexDataBuffer);
    free(b->m_indexDataBuffer);

    glDeleteVertexArrays(1, &b->m_glVAO);
    glDeleteBuffers(2, &b->m_glVBO);

    free(b);
}

void prwmbReset(PRWmeshBuilder* builder)
{
    getMeshBuilder;

    b->m_vertexDataPos = 0;
    b->m_indexDataPos = 0;
    b->m_numVerticies = 0;
    b->m_numIndicies = 0;
}

void prwmbDrawArrays(PRWmeshBuilder* builder, int mode)
{
    getMeshBuilder;

    glBindBuffer(GL_ARRAY_BUFFER, b->m_glVBO);
    if(b->m_vertexDataBufferCapacity <= b->m_glVertexBufferSize)
    {
#ifdef EMSCRIPTEN
        // Workaround. WebGL outputs a buffer overflow error when using glBufferSubData.
        glBufferData(GL_ARRAY_BUFFER, b->m_glVertexBufferSize, b->m_vertexDataBuffer, GL_DYNAMIC_DRAW);
#else
        glBufferSubData(GL_ARRAY_BUFFER, 0, b->m_vertexDataPos, b->m_vertexDataBuffer);
#endif
    }
    else
    {
        glBufferData(GL_ARRAY_BUFFER, b->m_vertexDataBufferCapacity, b->m_vertexDataBuffer, GL_DYNAMIC_DRAW);
        b->m_glVertexBufferSize = b->m_vertexDataBufferCapacity;
    }

    glBindVertexArray(b->m_glVAO);
    glDrawArrays(mode, 0, b->m_numVerticies);
    glBindVertexArray(0);
}

void prwmbDrawElements(PRWmeshBuilder* builder, int mode)
{
    getMeshBuilder;

    glBindBuffer(GL_ARRAY_BUFFER, b->m_glVBO);
    if(b->m_vertexDataBufferCapacity <= b->m_glVertexBufferSize)
    {
#ifdef EMSCRIPTEN
        // Workaround. WebGL outputs a buffer overflow error when using glBufferSubData.
        glBufferData(GL_ARRAY_BUFFER, b->m_glVertexBufferSize, b->m_vertexDataBuffer, GL_DYNAMIC_DRAW);
#else
        glBufferSubData(GL_ARRAY_BUFFER, 0, b->m_vertexDataPos, b->m_vertexDataBuffer);
#endif
    }
    else
    {
        glBufferData(GL_ARRAY_BUFFER, b->m_vertexDataBufferCapacity, b->m_vertexDataBuffer, GL_DYNAMIC_DRAW);
        b->m_glVertexBufferSize = b->m_vertexDataBufferCapacity;
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b->m_glEBO);
    if(b->m_indexDataBufferCapacity <= b->m_glElementBufferSize)
    {
#ifdef EMSCRIPTEN
        // Workaround. WebGL outputs a buffer overflow error when using glBufferSubData.
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, b->m_glElementBufferSize, b->m_indexDataBuffer, GL_DYNAMIC_DRAW);
#else
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, b->m_indexDataPos, b->m_indexDataBuffer);
#endif
    }
    else
    {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, b->m_indexDataBufferCapacity, b->m_indexDataBuffer, GL_DYNAMIC_DRAW);
        b->m_glElementBufferSize = b->m_indexDataBufferCapacity;
    }

    glBindVertexArray(b->m_glVAO);
    glDrawElements(mode, b->m_numIndicies, GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);
}

void prwmbPosition(PRWmeshBuilder* builder, float x, float y, float z)
{
    getMeshBuilder;

    vec3 pos = { x, y, z };
    glm_mat4_mulv3(*b->m_currentModelView, pos, 1.0f, pos);

    i_mbPushVertexData(b, sizeof(pos), pos);

    b->m_numVerticies++;
}

void prwmbNormal(PRWmeshBuilder* builder, float x, float y, float z)
{
    getMeshBuilder;

    vec3 normal = { x, y, z };
    i_mbMat4Get3x3(b);

    glm_mat3_mulv(b->m_modelView3x3, normal, normal);

    i_mbPushVertexData(b, sizeof(normal), normal);
}

void prwmbNormalDefault(PRWmeshBuilder* builder)
{
    prwmbNormal(builder, builder->defaultNormal[0], builder->defaultNormal[1], builder->defaultNormal[2]);
}

void prwmbUV(PRWmeshBuilder* builder, float u, float v)
{
    getMeshBuilder;

    vec2 uv = { u, v };

    i_mbPushVertexData(b, sizeof(uv), uv);
}

void prwmbUVDefault(PRWmeshBuilder* builder)
{
    prwmbUV(builder, builder->defualtUV[0], builder->defualtUV[1]);
}

void prwmbColorRGB(PRWmeshBuilder* builder, float r, float g, float b)
{
    prwmbColorRGBA(builder, r, g, b, builder->defaultColor[3]);
}

void prwmbColorRGBA(PRWmeshBuilder* builder, float r, float g, float blue, float a)
{
    getMeshBuilder;

    vec4 rgba = { r, g, blue, a };

    i_mbPushVertexData(b, sizeof(rgba), rgba);
}

void prwmbColorDefault(PRWmeshBuilder* builder)
{
    prwmbColorRGBA(builder, builder->defaultColor[0], builder->defaultColor[1], builder->defaultColor[2], builder->defaultColor[3]);
}

void prwmbVertex(PRWmeshBuilder* builder, ...)
{
    getMeshBuilder;

    prwvfVTXFMT* vtxFmt = &b->m_vertexFormat;
    uint8_t newVtxBuffer[1024];
    int newVtxBufferPos = 0;

    uint32_t numAttributes = (*vtxFmt)[0];
    uint32_t vertexSize = prwvfVertexNumBytes(*vtxFmt);

    va_list attribArgs;
    va_start(attribArgs, builder);

    float valuef;
    double valued;
    int32_t value4b;
    int16_t value2b;
    int8_t value1b;
    float* posInVertex = NULL;

    for(uint32_t i = 1; i <= numAttributes; i++)
    {
        uint32_t attribType = (*vtxFmt)[i] & PRWVF_ATTRB_TYPE_MASK;
        uint32_t attribSize = prwvfaGetSize((*vtxFmt)[i]);
        uint32_t attribUsage = (*vtxFmt)[i] & PRWVF_ATTRB_USAGE_MASK;
 
        if(attribUsage == PRWVF_ATTRB_USAGE_POS)
        {
            posInVertex = (float*) (newVtxBuffer + newVtxBufferPos);
        }

        for(uint32_t j = 0; j < attribSize; j++)
        {
            switch (attribType)
            {
            case PRWVF_ATTRB_TYPE_INT:
            case PRWVF_ATTRB_TYPE_UINT:
                value4b = va_arg(attribArgs, int32_t);
                memcpy(newVtxBuffer + newVtxBufferPos, &value4b, sizeof(int32_t));
                newVtxBufferPos += sizeof(int32_t);
                break;
            case PRWVF_ATTRB_TYPE_SHORT:
            case PRWVF_ATTRB_TYPE_USHORT:
                value2b = va_arg(attribArgs, int32_t);
                memcpy(newVtxBuffer + newVtxBufferPos, &value2b, sizeof(int16_t));
                newVtxBufferPos += sizeof(int16_t);
                break;
            case PRWVF_ATTRB_TYPE_BYTE:
            case PRWVF_ATTRB_TYPE_UBYTE:
                value1b = va_arg(attribArgs, int32_t);
                memcpy(newVtxBuffer + newVtxBufferPos, &value1b, sizeof(int8_t));
                newVtxBufferPos += sizeof(uint8_t);
                break;
            case PRWVF_ATTRB_TYPE_FLOAT:
                valuef = va_arg(attribArgs, double);
                memcpy(newVtxBuffer + newVtxBufferPos, &valuef, sizeof(float));
                newVtxBufferPos += sizeof(float);
                break;
            case PRWVF_ATTRB_TYPE_DOUBLE:
                valued = va_arg(attribArgs, double);
                memcpy(newVtxBuffer + newVtxBufferPos, &valued, sizeof(double));
                newVtxBufferPos += sizeof(double);
                break;
            }
        }
    }

    va_end(attribArgs);

    if(posInVertex)
    {
        glm_mat4_mulv3(*b->m_currentModelView, posInVertex, 1.0f, posInVertex);
    }

    i_mbPushVertexData(b, vertexSize, newVtxBuffer);
    b->m_numVerticies++;
}

void prwmbIndex(PRWmeshBuilder* builder, size_t numIndicies, ...)
{
    getMeshBuilder;

    uint32_t indicies[64];
    va_list indexArgs;
    va_start(indexArgs, numIndicies);

    numIndicies = 64 < numIndicies ? 64 : numIndicies;

    for(size_t i = 0; i < numIndicies; i++)
    {
        indicies[i] = va_arg(indexArgs, uint32_t) + b->m_numVerticies;
    }

    va_end(indexArgs);

    i_mbPushIndexData(b, numIndicies * sizeof(uint32_t), indicies);
    b->m_numIndicies += numIndicies;
}

void prwmbIndexv(PRWmeshBuilder* builder, size_t numIndicies, const uint32_t* indicies)
{
    getMeshBuilder;

    uint32_t bufferedIndicies[128];
    size_t bufferedIndiciesPos = 0;

    for(size_t i = 0; i < numIndicies; i++)
    {
        bufferedIndicies[bufferedIndiciesPos++] = indicies[i] + b->m_numVerticies;

        if(128 <= bufferedIndiciesPos)
        {
            i_mbPushIndexData(b, bufferedIndiciesPos * sizeof(uint32_t), bufferedIndicies);
            bufferedIndiciesPos = 0;
        }
    }

    if(0 < bufferedIndiciesPos)
    {
        i_mbPushIndexData(b, bufferedIndiciesPos * sizeof(uint32_t), bufferedIndicies);
    }

    b->m_numIndicies += numIndicies;
}

void prwmbTexid(PRWmeshBuilder* builder, uint32_t texid)
{
    getMeshBuilder;

    i_mbPushVertexData(b, sizeof(texid), &texid);
}

prwvfVTXFMT* prwmbGetVertexFormat(PRWmeshBuilder* builder)
{
    getMeshBuilder;

    return &(b->m_vertexFormat);
}

void prwmbPushMatrix(PRWmeshBuilder* builder)
{
    getMeshBuilder;

    if(PRWMB_MODELVIEW_STACK_SIZE <= b->m_stackIndex + 1)
    {
        return;
    }

    b->m_stackIndex++;
    b->m_currentModelView++;

    glm_mat4_copy(*(b->m_currentModelView - 1), *b->m_currentModelView);
}

void prwmbPopMatrix(PRWmeshBuilder* builder)
{
    getMeshBuilder;

    if(b->m_stackIndex - 1 < 0)
    {
        return;
    }

    b->m_stackIndex--;
    b->m_currentModelView--;
}

void prwmbResetMatrixStack(PRWmeshBuilder* builder)
{
    getMeshBuilder;

    b->m_currentModelView = b->m_modelViewStack;
    b->m_stackIndex = 0;
    glm_mat4_identity(*b->m_currentModelView);
    glm_mat3_identity(b->m_modelView3x3);
}

vec4* prwmbGetModelView(PRWmeshBuilder* builder)
{
    getMeshBuilder;

    return *b->m_currentModelView;
}

const uint8_t* prwmbGetVertexBuffer(PRWmeshBuilder* builder, size_t* getNumBytes)
{
    getMeshBuilder;

    if(getNumBytes)
    {
        *getNumBytes = b->m_vertexDataPos;
    }

    return b->m_vertexDataBuffer;
}

const uint32_t* prwmbGetIndexBuffer(PRWmeshBuilder* builder, size_t* getNumBytes)
{
    getMeshBuilder;

    if(getNumBytes)
    {
        *getNumBytes = b->m_indexDataPos;
    }

    return b->m_indexDataBuffer;
}

static void i_mbPushVertexData(MeshBuilder* b, size_t size, void* data)
{
    if(b->m_vertexDataBufferCapacity < b->m_vertexDataPos + size)
    {
        size_t newCapactiy = b->m_vertexDataBufferCapacity * 2;
        b->m_vertexDataBuffer = realloc(b->m_vertexDataBuffer, newCapactiy);
        b->m_vertexDataBufferCapacity = newCapactiy;
    }

    memcpy(b->m_vertexDataBuffer + b->m_vertexDataPos, data, size);
    b->m_vertexDataPos += size;
}

static void i_mbPushIndexData(MeshBuilder* b, size_t size, void* data)
{
    if(b->m_indexDataBufferCapacity < b->m_indexDataPos + size)
    {
        size_t newCapactiy = b->m_indexDataBufferCapacity * 2;
        b->m_indexDataBuffer = realloc(b->m_indexDataBuffer, newCapactiy);
        b->m_indexDataBufferCapacity = newCapactiy;
    }

    uint8_t* indexBuffer = (uint8_t*) b->m_indexDataBuffer;

    memcpy(indexBuffer + b->m_indexDataPos, data, size);
    b->m_indexDataPos += size;
}

static void i_mbMat4Get3x3(MeshBuilder* b)
{
    mat4* m_4x4 = b->m_currentModelView;
    mat3* m_3x3 = &b->m_modelView3x3;

    for(int r = 0; r < 3; r++)
    {
        for(int c = 0; c < 3; c++)
        {
            (*m_3x3)[r][c] = (*m_4x4)[r][c];
        }
    }
}