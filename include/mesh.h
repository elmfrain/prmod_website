#pragma once

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

void prwmMeshRenderv(PRWmesh* mesh);

void prwmMeshRender(const char* meshName);

void prwmRemovev(PRWmesh* mesh);

void prwmRemove(const char* meshName);