#pragma once

#define PRWM_VERTF_POS 0
#define PRWM_VERTF_POS_UV 1
#define PRWM_VERTF_POS_COLOR 2
#define PRWM_VERTF_POS_UV_COLOR 3
#define PRWM_VERTF_POS_UV_COLOR_NORM 4

typedef struct PRWmesh
{
    int nbIndicies;
    int vertexFormat;
    char name[128];
} PRWmesh;

void prwmLoad(const char* filename);

PRWmesh* prwmMeshGet(const char* meshName);

void prwmMeshRenderv(PRWmesh* mesh);

void prwmMeshRender(const char* meshName);

void prwmRemovev(PRWmesh* mesh);

void prwmRemove(const char* meshName);