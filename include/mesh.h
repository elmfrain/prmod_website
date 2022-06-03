#pragma once

#include "mesh_builder.h"

typedef struct PRWmesh
{
    char name[256];
    float* positions;
    size_t numVerticies;
    float* uvs;
    float* normals;
    float* colors;
    uint32_t* indicies;
    size_t numIndicies;
} PRWmesh;

void prwmLoad(const char* filename);

PRWmesh* prwmMeshGet(const char* meshName);

uint32_t prwmGetGLTexture(PRWmesh* mesh);

// deprecated: use prwmPutMesh(PRWmeshBuilder, PRWmesh) instead
void prwmMeshRenderv(PRWmesh* mesh);

// deprecated: use prwmPutMesh(PRWmeshBuilder, PRWmesh) instead
void prwmMeshRender(const char* meshName);

void prwmPutMeshArrays(PRWmeshBuilder* meshBuilder, PRWmesh* mesh);

void prwmPutMeshElements(PRWmeshBuilder* meshBuilder, PRWmesh* mesh);

void prwmRemovev(PRWmesh* mesh);

void prwmRemove(const char* meshName);