#pragma once

#include <cglm/mat4.h>

#ifndef EMSCRIPTEN
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#endif

//-----------------------VertexFormat Class--------------------------

// Vertex Attribute
typedef uint32_t prwvfVTXATTRB;

// Vertex Format
typedef prwvfVTXATTRB prwvfVTXFMT[32];

// Vertex Attribute Members Masks
#define PRWVF_ATTRB_USAGE_MASK      0xFF000000
#define PRWVF_ATTRB_TYPE_SIZE_MASK  0x00F00000
#define PRWVF_ATTRB_TYPE_MASK       0x00FF0000
#define PRWVF_ATTRB_SIZE_MASK       0x0000FF00
#define PRWVF_ATTRB_NORMALIZED_MASK 0x000000FF

// Vertex Attribute Usage Enum
#define PRWVF_ATTRB_USAGE_POS       0x00000000 // Position, enum value 0
#define PRWVF_ATTRB_USAGE_UV        0x01000000 // UV,       enum value 1
#define PRWVF_ATTRB_USAGE_COLOR     0x02000000 // Color,    enum value 2
#define PRWVF_ATTRB_USAGE_NORMAL    0x03000000 // Normal,   enum value 3
#define PRWVF_ATTRB_USAGE_TEXID     0x04000000 // Texid,    enum value 4
#define PRWVF_ATTRB_USAGE_OTHER     0x05000000 // Other,    enum value 5

// Vertex Attribute Type Enum
#define PRWVF_ATTRB_TYPE_FLOAT      0x00400000 // Float           , size 4, enum value 0
#define PRWVF_ATTRB_TYPE_INT        0x00410000 // Integer         , size 4, enum value 1
#define PRWVF_ATTRB_TYPE_UINT       0x00420000 // Unsigned Integer, size 4, enum value 2
#define PRWVF_ATTRB_TYPE_SHORT      0x00230000 // Short           , size 2, enum value 3
#define PRWVF_ATTRB_TYPE_USHORT     0x00240000 // Unsigned Short  , size 2, enum value 4
#define PRWVF_ATTRB_TYPE_BYTE       0x00150000 // Byte            , size 1, enum value 5
#define PRWVF_ATTRB_TYPE_UBYTE      0x00160000 // Unsigned Byte   , size 1, enum value 6
#define PRWVF_ATTRB_TYPE_DOUBLE     0x00870000 // Double          , size 8, enum value 7

// Vertex Attribute Size
#define PRWVF_ATTRB_SIZE(x) ((uint32_t) x) << 8

// Vertex Normalized
#define PRWVF_ATTRB_NORMALIZED_TRUE 1
#define PRWVF_ATTRB_NORMALIZED_FALSE 0

void prwvfApply(prwvfVTXFMT vtxFmt);

void prwvfUnapply(prwvfVTXFMT vtxFmt);

uint32_t prwvfVertexNumBytes(prwvfVTXFMT vtxFmt);

uint32_t prwvfaGetUsage(prwvfVTXATTRB attrib);

uint32_t prwvfaGetSize(prwvfVTXATTRB attrib);

uint32_t prwvfaGetType(prwvfVTXATTRB attrib);

uint32_t prwvfaGetGLType(prwvfVTXATTRB attrib);

uint32_t prwvfaNumBytes(prwvfVTXATTRB attrib);

bool prwvfaIsNormalized(prwvfVTXATTRB attrib);

//-----------------------MeshBuilder Class---------------------------

typedef struct PRWmeshBuilder
{
    float defaultNormal[3];
    float defualtUV[2];
    float defaultColor[4];
} PRWmeshBuilder;

PRWmeshBuilder* prwmbGenBuilder(prwvfVTXFMT vertexFormat);

void prwmbDeleteBuilder(PRWmeshBuilder* builder);

void prwmbReset(PRWmeshBuilder* builder);

void prwmbDrawArrays(PRWmeshBuilder* builder,  int mode);

void prwmbDrawElements(PRWmeshBuilder* builder, int mode);

void prwmbPosition(PRWmeshBuilder* builder, float x, float y, float z);

void prwmbNormal(PRWmeshBuilder* builder, float x, float y, float z);

void prwmbNormalDefault(PRWmeshBuilder* builder);

void prwmbUV(PRWmeshBuilder* builder, float u, float v);

void prwmbUVDefault(PRWmeshBuilder* builder);

void prwmbColorRGB(PRWmeshBuilder* builder, float r, float g, float b);

void prwmbColorRGBA(PRWmeshBuilder* builder, float r, float g, float b, float a);

void prwmbColorDefault(PRWmeshBuilder* builder);

void prwmbIndex(PRWmeshBuilder* builder, size_t numIndicies, ...);

void prwmbTexid(PRWmeshBuilder* builder, uint8_t texid);

prwvfVTXFMT* prwmbGetVertexFormat(PRWmeshBuilder* builder);

void prwmbPushMatrix(PRWmeshBuilder* builder);

void prwmbPopMatrix(PRWmeshBuilder* builder);

void prwmbResetMatrixStack(PRWmeshBuilder* builder);

vec4* prwmbGetModelView(PRWmeshBuilder* builder);