#include "mesh.h"

#include <glad/glad.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Mesh
{
    int nbIndicies;
    int vertexFormat;
    char name[128];
    GLuint glVAO;
    GLuint glVBO;
    GLuint glEBO;
    GLuint glTexture;
} Mesh;

//Internal Mesh list
static int m_listSize = 0;
static int m_listCapacity = 32;
static Mesh** m_meshList = NULL;
static Mesh* i_lAdd(Mesh mesh);
static void i_lRemove(Mesh* mesh);

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
            if(i + 1 < m_listSize) memcpy(&m_meshList[i], &m_meshList[i + 1], m_listSize - (i + 1));
            m_listSize--;
        }
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
    else printf("[BIN Mesh Loader][Error]: Unable to open %s\n", filename);
}