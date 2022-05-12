#pragma once

#include "mesh_builder.h"

typedef struct PRWmesh
{
    char name[256];
    float* positions;
    size_t positionsSize;
    float* uvs;
    size_t uvsSize;
    float* normals;
    size_t normalsSize;
    float* colors;
    size_t colorsSize;
    uint32_t* indicies;
    size_t indiciesSize;
} PRWmesh;

void prwmLoad(const char* filename);

PRWmesh* prwmMeshGet(const char* meshName);

uint32_t prwmGetGLTexture(PRWmesh* mesh);

// deprecated: use prwmPutMesh(PRWmeshBuilder, PRWmesh) instead
void prwmMeshRenderv(PRWmesh* mesh);

// deprecated: use prwmPutMesh(PRWmeshBuilder, PRWmesh) instead
void prwmMeshRender(const char* meshName);

void prwmPutMesh(PRWmeshBuilder* meshBuilder, PRWmesh* mesh);

void prwmRemovev(PRWmesh* mesh);

void prwmRemove(const char* meshName);