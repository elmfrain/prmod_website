#include "background_renderer.h"

#include "shaders.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cglm/vec3.h>
#include <cglm/vec2.h>

#include <glad/glad.h>

struct MeshHeader
{
    char name[64];
    int verticiesArrayPtr, verticiesArraySize;
	int indiciesArrayPtr, indiciesArraySize;
	int texturePtr, textureWidth, textureHeight;
} MeshHeader;

struct Vertex
{
    vec3 pos;
    vec2 uv;
} Vertex;

struct Mesh
{
    GLuint glBufferID;
    GLuint glElemBufferID;
    GLuint glVertArrayID;
    GLuint glTextureID;
    GLuint vertexCount;
} Mesh;

static struct Mesh CUBE_MAP_MESH;

static char m_isLoaded = 0;
static int m_nbHeaders = 0;
static struct MeshHeader* m_headers = NULL;

static void i_loadMeshes(const char** buffer, size_t buffernLen);
static struct Mesh i_createMesh(const char** bufferPtrs, size_t* sizes);
static void i_loadMeshHeaders(const char** buffer, size_t bufferLen);
static struct MeshHeader i_readMeshHeader(const char** buffer, size_t bufferLen);
static void i_addMeshHeader(struct MeshHeader header);
static void i_clearMeshHeaders();
static void i_renderMesh(struct Mesh mesh, GLenum drawMode);

int prwbrIsLoaded()
{
    return m_isLoaded;
}

int prwbrLoad()
{
    FILE* file = fopen("res/cube_map.bin", "rb");

    if(file)
    {
        fseek(file, 0, SEEK_END);
        size_t fileSize = ftell(file);

        const char* bufferedInput = malloc(fileSize);
        rewind(file);
        fread((char*) bufferedInput, fileSize, 1, file);
        fclose(file);

        const char* bufferReadPtr = bufferedInput;
        i_loadMeshHeaders(&bufferReadPtr, fileSize);
        i_loadMeshes(&bufferedInput, fileSize);
        m_isLoaded = true;

        free((char*) bufferedInput);
    }
}

int prwbrRender()
{
    if(!m_isLoaded) return 0;

    i_renderMesh(CUBE_MAP_MESH, GL_TRIANGLES);

    return 1;
}

static void i_loadMeshes(const char** buffer, size_t buffernLen)
{
    const char* beginning = *buffer;

    for(int i = 0; i < m_nbHeaders; i++)
    {
        struct MeshHeader* header = &m_headers[i];
        if(strcmp(header->name, "cube_map") == 0)
        {
            const char* bufferPtrs[3];
            size_t sizes[4];

            bufferPtrs[0] = beginning + header->verticiesArrayPtr;
            bufferPtrs[1] = beginning + header->indiciesArrayPtr;
            bufferPtrs[2] = beginning + header->texturePtr;
            sizes[0] = header->verticiesArraySize * sizeof(struct Vertex);
            sizes[1] = header->indiciesArraySize * sizeof(int);
            sizes[2] = header->textureWidth;
            sizes[3] = header->textureHeight;

            CUBE_MAP_MESH = i_createMesh(bufferPtrs, sizes);
        }
    }
}

static struct Mesh i_createMesh(const char** bufferPtrs, size_t* sizes)
{
    struct Mesh mesh;

    const char* vertexData = bufferPtrs[0];  size_t vertexDataSize = sizes[0];
    const char* indexData = bufferPtrs[1];   size_t indexDataSize = sizes[1];
    const char* textureData = bufferPtrs[2];
    size_t textureWidth = sizes[2], textureHeight = sizes[3];
    mesh.vertexCount = indexDataSize / sizeof(int);

    glGenBuffers(2, &mesh.glBufferID);

    glGenVertexArrays(1, &mesh.glVertArrayID);

    glBindVertexArray(mesh.glVertArrayID);
    {
        glBindBuffer(GL_ARRAY_BUFFER, mesh.glBufferID);
        glBufferData(GL_ARRAY_BUFFER, vertexDataSize, vertexData, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.glElemBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSize, indexData, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), &((struct Vertex*)0)->uv);
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glGenTextures(1, &mesh.glTextureID);
    glBindTexture(GL_TEXTURE_2D, mesh.glTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);

    return mesh;
}

static void i_loadMeshHeaders(const char** buffer, size_t bufferLen)
{
    i_clearMeshHeaders();

    while(**buffer) i_addMeshHeader(i_readMeshHeader(buffer, bufferLen));
}

static struct MeshHeader i_readMeshHeader(const char** buffer, size_t bufferLen)
{
    struct MeshHeader header;

    const char* prevPos = *buffer;
    int nameSize = 0;
    for(int i = 0 ; i < bufferLen; i++)
    {
        if(**buffer)
        {
            nameSize++;
            (*buffer)++;
        }
        else break;
    }

    (*buffer)++;
    memset(header.name, 0, 64);
    memcpy(header.name, prevPos, nameSize < 64 ? nameSize : 63);
    
    header.verticiesArrayPtr =  *((int*)(*buffer)); *buffer += 4;
    header.verticiesArraySize = *((int*)(*buffer)); *buffer += 4;
    header.indiciesArrayPtr =   *((int*)(*buffer)); *buffer += 4;
    header.indiciesArraySize =  *((int*)(*buffer)); *buffer += 4;
    header.texturePtr =         *((int*)(*buffer)); *buffer += 4;
    header.textureWidth =       *((int*)(*buffer)); *buffer += 4;
    header.textureHeight =      *((int*)(*buffer)); *buffer += 4;
    
    return header;
}

static void i_addMeshHeader(struct MeshHeader header)
{
    if(!m_headers) m_headers = malloc(sizeof(struct MeshHeader));
    else m_headers = realloc(m_headers, (m_nbHeaders + 1) * sizeof(struct MeshHeader));

    m_headers[m_nbHeaders] = header;

    m_nbHeaders++;
}

static void i_clearMeshHeaders()
{
    free(m_headers);
    m_headers = NULL;
    m_nbHeaders = 0;
}


static void i_renderMesh(struct Mesh mesh, GLenum drawMode)
{
    prws_POS_UV_shader();

    glBindTexture(GL_TEXTURE_2D, mesh.glTextureID);

    glBindVertexArray(mesh.glVertArrayID);
    glDrawElements(drawMode, mesh.vertexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
