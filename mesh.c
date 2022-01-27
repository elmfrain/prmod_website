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
    int nbIndicies;
    int vertexFormat;
    char name[1024];
    GLuint glVAO;
    GLuint glVBO;
    GLuint glEBO;
    GLuint glTexture;
} Mesh;

struct pos_uvVertex
{
    struct aiVector3D pos;
    struct aiVector2D uv;
} pos_uvVertex;
struct face
{
    int indicies[3];
};

//Internal Mesh list
static int m_listSize = 0;
static int m_listCapacity = 32;
static Mesh** m_meshList = NULL;
static Mesh* i_lAdd(Mesh mesh);
static void i_lRemove(Mesh* mesh);

static void i_meshLoadAssimp(const char* filename);
static void i_meshLoadBIN(const char* filename);

void prwmLoad(const char* filename)
{
    char extension[24] = { 0 };
    for(int i = strlen(filename) - 1; i >= 0; i--)
        if(filename[i] == '.')
        {
            strcpy(extension, filename + i + 1);
            break;
        }

    if(strcmp(extension, "bin") == 0) 
        i_meshLoadBIN(filename);
    else i_meshLoadAssimp(filename);
}

PRWmesh* prwmMeshGet(const char* meshName)
{
    for(int i = 0; i < m_listSize; i++)
        if(strcmp(m_meshList[i]->name, meshName) == 0) return (PRWmesh*) m_meshList[i];

    return NULL;
}

void prwmMeshRenderv(PRWmesh* mesh)
{
    Mesh* m = (Mesh*) mesh;
    if(!mesh) return;

    if(m->glTexture)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m->glTexture);
    }

    glBindVertexArray(m->glVAO);
    glDrawElements(GL_TRIANGLES, m->nbIndicies, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void prwmMeshRender(const char* meshName)
{
    prwmMeshRenderv(prwmMeshGet(meshName));
}

void prwmRemovev(PRWmesh* mesh)
{
    Mesh* m = (Mesh*) mesh;
    glDeleteVertexArrays(1, &m->glVAO);
    glDeleteBuffers(2, &m->glVBO);
    if(m->glTexture) glDeleteTextures(1, &m->glTexture);

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

        strcpy(newMesh.name, mesh->mName.data);

        //Collect vertex data
        struct pos_uvVertex* vertexData = malloc(sizeof(struct pos_uvVertex) * mesh->mNumVertices);
        for(int j = 0; j < mesh->mNumVertices; j++)
        {
            vertexData[j].pos = mesh->mVertices[j];
            struct aiVector3D uv = mesh->mTextureCoords[0][j];
            vertexData[j].uv.x = uv.x;
            vertexData[j].uv.y = uv.y;
        }

        //Collect face data
        newMesh.nbIndicies = mesh->mNumFaces * 3;
        struct face* indexData = malloc(sizeof(struct face) * mesh->mNumFaces);
        for(int j = 0; j < mesh->mNumFaces; j++)
        {
            indexData[j].indicies[0] = mesh->mFaces[j].mIndices[0];
            indexData[j].indicies[1] = mesh->mFaces[j].mIndices[1];
            indexData[j].indicies[2] = mesh->mFaces[j].mIndices[2];
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

        //Load to OpenGL
        glGenVertexArrays(1, &newMesh.glVAO);
        glGenBuffers(2, &newMesh.glVBO);
        glBindVertexArray(newMesh.glVAO);
        {
            glBindBuffer(GL_ARRAY_BUFFER, newMesh.glVBO);
            glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(struct pos_uvVertex), vertexData, GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newMesh.glEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->mNumFaces * sizeof(struct face), indexData, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 20, 0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 20, (void*) 12);
        }
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        if(image)
        {
            glGenTextures(1, &newMesh.glTexture);
            glBindTexture(GL_TEXTURE_2D, newMesh.glTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
            stbi_image_free(image);
        }

        free(vertexData);
        free(indexData);
        i_lAdd(newMesh);
    }
}

static void i_meshLoadBIN(const char* filename)
{
    struct MeshHeader
    {
        int verticiesArrayPtr, verticiesArraySize;
	    int indiciesArrayPtr, indiciesArraySize;
	    int texturePtr, textureWidth, textureHeight;
    } MeshHeader;

    FILE* file = fopen(filename, "rb");

    if(file)
    {
        fseek(file, 0, SEEK_END);
        size_t fileSize = ftell(file);

        char* bufferedInput = malloc(fileSize);
        rewind(file);
        fread((char*) bufferedInput, fileSize, 1, file);
        fclose(file);

        char* bufferReadPtr = bufferedInput;
    createMesh:
        {
            if(!*bufferReadPtr)
            {
                free(bufferedInput);
                return;
            }

            Mesh mesh;
            struct MeshHeader header;
            memset(mesh.name, 0, sizeof(mesh.name));
            mesh.vertexFormat = PRWM_VERTF_POS_UV;

            strncpy(mesh.name, bufferReadPtr, sizeof(mesh.name) - 1);
            bufferReadPtr += strlen(bufferReadPtr) + 1;

            memcpy(&header.verticiesArrayPtr, bufferReadPtr, sizeof(int32_t) * 7);
            bufferReadPtr += sizeof(int32_t) * 7;

            const char* vertexData = bufferedInput + header.verticiesArrayPtr;
            const char* indexData = bufferedInput + header.indiciesArrayPtr;
            const char* textureData = bufferedInput + header.texturePtr;
            mesh.nbIndicies = header.indiciesArraySize;

            glGenVertexArrays(1, &mesh.glVAO);
            glGenBuffers(2, &mesh.glVBO);
            glGenTextures(1, &mesh.glTexture);

            glBindVertexArray(mesh.glVAO);
            {
                glBindBuffer(GL_ARRAY_BUFFER, mesh.glVBO);
                glBufferData(GL_ARRAY_BUFFER, header.verticiesArraySize * 20, vertexData, GL_STATIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.glEBO);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, header.indiciesArraySize * sizeof(uint32_t), indexData, GL_STATIC_DRAW);

                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 20, 0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 20, (void*) 12);
            }
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

            glBindTexture(GL_TEXTURE_2D, mesh.glTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, header.textureWidth, header.textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);

            i_lAdd(mesh);
            goto createMesh;
        }
    }
    else printf("[BIN Mesh Loader][Error]: Unable to load %s\n", filename);
}