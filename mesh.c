#include "mesh.h"

#ifndef EMSCRIPTEN
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#endif

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

#include <stb_image.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Mesh
{
    PRWmesh mesh;
    GLuint m_glTexture;
} Mesh;

//Internal Mesh list
static int m_listSize = 0;
static int m_listCapacity = 32;
static Mesh** m_meshList = NULL;
static Mesh* i_lAdd(Mesh mesh);
static void i_lRemove(Mesh* mesh);

//Mesh Renderer (Deprecated)
PRWmeshBuilder* m_meshRenderer = NULL;

static void i_meshLoadAssimp(const char* filename);

void prwmLoad(const char* filename)
{
    i_meshLoadAssimp(filename);
}

PRWmesh* prwmMeshGet(const char* meshName)
{
    for(int i = 0; i < m_listSize; i++)
        if(strcmp(m_meshList[i]->mesh.name, meshName) == 0) return (PRWmesh*) m_meshList[i];

    return NULL;
}

void prwmMeshRenderv(PRWmesh* mesh)
{
    Mesh* m = (Mesh*) mesh;
    if(!mesh) return;

    if(!m_meshRenderer)
    {
        prwvfVTXFMT vtxFmt;
        vtxFmt[0] = 2; // Number of attributes

        vtxFmt[1] = PRWVF_ATTRB_USAGE_POS 
                  | PRWVF_ATTRB_TYPE_FLOAT
                  | PRWVF_ATTRB_SIZE(3)
                  | PRWVF_ATTRB_NORMALIZED_FALSE;

        vtxFmt[2] = PRWVF_ATTRB_USAGE_UV
                  | PRWVF_ATTRB_TYPE_FLOAT
                  | PRWVF_ATTRB_SIZE(2)
                  | PRWVF_ATTRB_NORMALIZED_FALSE;

        m_meshRenderer = prwmbGenBuilder(vtxFmt);
    }

    if(m->m_glTexture)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m->m_glTexture);
    }

    prwmbReset(m_meshRenderer);
    uint32_t numIndicies = m->mesh.numIndicies;
    float defaultUV[] = { 0.0f, 0.0f };

    for(uint32_t i = 0; i < numIndicies; i++)
    {
        uint32_t index = m->mesh.indicies[i];
        float* pos = &m->mesh.positions[index * 3];
        float* uv = m->mesh.uvs ?  &m->mesh.uvs[index * 2] : defaultUV;

        prwmbVertex(m_meshRenderer, pos[0], pos[1], pos[2], uv[0], uv[1]);
    }

    prwmbDrawArrays(m_meshRenderer, GL_TRIANGLES);
}

void prwmMeshRender(const char* meshName)
{
    prwmMeshRenderv(prwmMeshGet(meshName));
}

void prwmRemovev(PRWmesh* mesh)
{
    Mesh* m = (Mesh*) mesh;
    if(m->m_glTexture) glDeleteTextures(1, &m->m_glTexture);
    if(m->mesh.positions) free(m->mesh.positions);
    if(m->mesh.uvs) free(m->mesh.uvs);
    if(m->mesh.normals) free(m->mesh.normals);
    if(m->mesh.colors) free(m->mesh.colors);
    if(m->mesh.indicies) free(m->mesh.indicies);

    i_lRemove(m);
}

void prwmRemove(const char* meshName)
{
    prwmRemovev(prwmMeshGet(meshName));
}

static Mesh* i_lAdd(Mesh mesh)
{
    Mesh* ptr = malloc(sizeof(Mesh));

    if(ptr)
    {
        Mesh** newListBlock = m_meshList;
        if(!m_meshList) newListBlock = malloc(sizeof(Mesh*) * m_listCapacity);
        else if((m_listSize + 1) > m_listCapacity)
        {
            newListBlock = realloc(m_meshList, m_listCapacity * 2);
            if(newListBlock) m_listCapacity *= 2;
        }

        if(!newListBlock) 
        {
            free(ptr);
            return NULL;
        }

        *ptr = mesh;

        m_meshList = newListBlock;

        m_meshList[m_listSize] = ptr;
        m_listSize++;
    }

    return ptr;
}

static void i_lRemove(Mesh* mesh)
{
    for(int i = 0; i < m_listSize; i++)
    {
        if(mesh == m_meshList[i])
        {
            free(m_meshList[i]);
            if(i + 1 < m_listSize) memcpy(&m_meshList[i], &m_meshList[i + 1], (m_listSize - (i + 1)) * sizeof(struct Mesh));
            m_listSize--;
        }
    }
}

static void i_meshLoadAssimp(const char* filename)
{
    char dir[1024] = {0};
    for(int i = strlen(filename); 0 <= i; i--)
        if(filename[i] == '/' || filename[i] == '\\')
        {
            strncpy(dir, filename, i + 1);
            break;
        }

    const struct aiScene* scene = aiImportFile(filename, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    if(!scene)
    {
        printf("[Mesh][Error]: Failed loading: \"%s\"\n", filename);
        return;
    }

    for(int i = 0; i < scene->mNumMeshes; i++)
    {
        struct Mesh newMesh;
        const struct aiMesh* mesh = scene->mMeshes[i];
        memset(&newMesh, 0, sizeof(Mesh));

        printf("[Mesh][Info]: Loading mesh \"%s\"\n", mesh->mName.data);

        strcpy(newMesh.mesh.name, mesh->mName.data);

        newMesh.mesh.numVerticies = mesh->mNumVertices;

        //Get position data
        newMesh.mesh.positions = malloc(sizeof(float) * mesh->mNumVertices * 3);
        memcpy(newMesh.mesh.positions, mesh->mVertices, sizeof(float) * mesh->mNumVertices * 3);

        //Get uv data
        if(mesh->mTextureCoords[0])
        {
            newMesh.mesh.uvs = malloc(sizeof(float) * mesh->mNumVertices * 2);
            for(int j = 0; j < mesh->mNumVertices; j++)
            {
                struct aiVector3D uv = mesh->mTextureCoords[0][j];
                newMesh.mesh.uvs[j * 2    ] = uv.x;
                newMesh.mesh.uvs[j * 2 + 1] = uv.y;
            }
        }
        else
        {
            newMesh.mesh.uvs = NULL;
        }

        //Get normal data
        newMesh.mesh.normals = malloc(sizeof(float) * mesh->mNumVertices * 3);
        memcpy(newMesh.mesh.normals, mesh->mNormals, sizeof(float) * mesh->mNumVertices * 3);

        //Get color data
        if(mesh->mColors[0])
        {
            newMesh.mesh.colors = malloc(sizeof(float) * mesh->mNumVertices * 4);
            memcpy(newMesh.mesh.colors, mesh->mColors[0], sizeof(float) * mesh->mNumVertices * 4);
        }
        else
        {
            newMesh.mesh.colors = NULL;
        }

        //Get index data
        newMesh.mesh.numIndicies = mesh->mNumFaces * 3;
        newMesh.mesh.indicies = malloc(sizeof(uint32_t) * newMesh.mesh.numIndicies);
        for(int j = 0; j < mesh->mNumFaces; j++)
        {
            uint32_t* faceIndicies = mesh->mFaces[j].mIndices;
            newMesh.mesh.indicies[j * 3    ] = faceIndicies[0];
            newMesh.mesh.indicies[j * 3 + 1] = faceIndicies[1];
            newMesh.mesh.indicies[j * 3 + 2] = faceIndicies[2];
        }

        //Get texture
        struct aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        stbi_uc* image = NULL; int width, height, channels;
        if(material && aiGetMaterialTextureCount(material, aiTextureType_DIFFUSE))
        {
            struct aiString textureName;
            aiGetMaterialTexture(material, aiTextureType_DIFFUSE, 0, &textureName, NULL, NULL, NULL, NULL, NULL, NULL);
            char textureFile[1024] = {0};
            strcat(textureFile, dir);
            strcat(textureFile, textureName.data);
            stbi_set_flip_vertically_on_load(1);
            image = stbi_load(textureFile, &width, &height, &channels, STBI_rgb_alpha);

            if(!image)
            {
                printf("[Mesh][Error]: Failed to load texture \"%s\" for mesh \"%s\"", textureFile, mesh->mName.data);
                return;
            }
        }

        newMesh.m_glTexture = 0;
        if(image)
        {
            glGenTextures(1, &newMesh.m_glTexture);
            glBindTexture(GL_TEXTURE_2D, newMesh.m_glTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
            stbi_image_free(image);
        }

        i_lAdd(newMesh);
    }
}