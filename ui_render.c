#include "ui_render.h"

#include "shaders.h"
#include "mesh_builder.h"

#include <cglm/cam.h>
#include <cglm/affine.h>

#include <stb_image.h>

#include <string.h>
#include <ctype.h>
#include <time.h>

#define getVec4Color(intcolor) { ((intcolor >> 16) & 0xFF) / 255.0f, ((intcolor >> 8) & 0xFF) / 255.0f, (intcolor & 0xFF) / 255.0f, ((intcolor >> 24) & 0xFF) / 255.0f }
#define vec4Color(vec4color) vec4color[0], vec4color[1], vec4color[2], vec4color[3]

static inline void i_orient(float* dst, int mode, float left, float top, float right, float bottom);

// Batch Rendering
static PRWmeshBuilder* m_meshBuilder;

// Windowing
static GLFWwindow* m_window = NULL;
static int m_windowWidth = 0;
static int m_windowHeight = 0;
static float m_UIscaleFctor = 2.0f;

static void i_onWindowResize(GLFWwindow*, int, int);
static void i_init();

static void i_frInit();
static void i_frBindAtlases();
static inline void i_frGenString(const char* str, float x, float y, uint32_t color);
static inline float i_frGetStringWidth(const char* str);
static inline float i_frGetCharWidth(int unicode);
static inline void i_frAnchor(int mode, const char* str, float x, float y, float* newpos);

#ifdef EMSCRIPTEN //Called when the canvas size changes in browser
void prwuiResize(int width, int height)
{
    if(!m_window) return;

    if(width < 128 || 8192 < width) width = 1280;
    if(height < 128 || 8192 < height) height = 720;
    glfwSetWindowSize(m_window, width, height);
}
#endif

void prwuiSetWindow(GLFWwindow* window)
{
    GLFWwindow* prevWindow = m_window;
    m_window = window;

    if(m_window)
    {
        glfwGetWindowSize(m_window, &m_windowWidth, &m_windowHeight);
        glfwSetWindowSizeCallback(m_window, i_onWindowResize);

        i_init();
    }
    else if(prevWindow)
    {
        glfwSetWindowSizeCallback(prevWindow, NULL);
        m_windowWidth = m_windowHeight = 0;
    }
}

void prwuiSetupUIrendering()
{
    mat4 projection; glm_ortho(0, prwuiGetUIwidth(), prwuiGetUIheight(), 0, -500, 500, projection);
    prwsSetProjectionMatrix(projection);

    glDisable(GL_DEPTH_TEST);
    prwsSetModelViewMatrix(prwmbGetModelView(m_meshBuilder));
}

void prwuiGenQuad(float left, float top, float right, float bottom, uint32_t color, int texID)
{
    texID = texID <= 16 ? texID : 0;

    vec4 colorv4 = getVec4Color(color);

    prwmbIndex(m_meshBuilder, 6, 0, 1, 2, 0, 2, 3);

    prwmbVertex(m_meshBuilder, left , bottom, 0.f, 0.f, 0.f, vec4Color(colorv4), texID);
    prwmbVertex(m_meshBuilder, right, bottom, 0.f, 1.f, 0.f, vec4Color(colorv4), texID);
    prwmbVertex(m_meshBuilder, right, top   , 0.f, 1.f, 1.f, vec4Color(colorv4), texID);
    prwmbVertex(m_meshBuilder, left , top   , 0.f, 0.f, 1.f, vec4Color(colorv4), texID);
}

void prwuiGenGradientQuad(int direction, float left, float top, float right, float bottom, uint32_t startColor, uint32_t endColor, int texID)
{
    texID = texID <= 16 ? texID : 0;

    vec4 sColorv4 = getVec4Color(startColor);
    vec4 eColorv4 = getVec4Color(endColor);

    float positions[8]; i_orient(positions, direction, left, top, right, bottom);

    prwmbIndex(m_meshBuilder, 6, 0, 1, 2, 0, 2, 3);

    prwmbVertex(m_meshBuilder, positions[0], positions[1], 0.f, 0.f, 0.f, vec4Color(eColorv4), texID);
    prwmbVertex(m_meshBuilder, positions[2], positions[3], 0.f, 1.f, 0.f, vec4Color(eColorv4), texID);
    prwmbVertex(m_meshBuilder, positions[4], positions[5], 0.f, 1.f, 1.f, vec4Color(sColorv4), texID);
    prwmbVertex(m_meshBuilder, positions[6], positions[7], 0.f, 0.f, 1.f, vec4Color(sColorv4), texID);
}

void prwuiGenVerticalLine(float x, float startY, float endY, uint32_t color)
{
    prwuiGenQuad(x, startY, x + 1, endY, color, 0);
}

void prwuiGenHorizontalLine(float y, float startX, float endX, uint32_t color)
{
    prwuiGenQuad(startX, y, endX, y + 1, color, 0);
}

void prwuiGenString(int anchor, const char* str, float x, float y, uint32_t color)
{
    vec2 newpos;
    i_frAnchor(anchor, str, x, y, newpos);

    i_frGenString(str, newpos[0], newpos[1], color);
}

void prwuiRenderBatch()
{
    prws_POS_UV_COLOR_TEXID_shader();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    i_frBindAtlases();

    prwmbDrawElements(m_meshBuilder, GL_TRIANGLES);

    prwmbReset(m_meshBuilder);
}

float prwuiGetStringHeight()
{
    return 9;
}

float prwuiGetStringWidth(const char* str)
{
    return i_frGetStringWidth(str);
}

float prwuiGetCharWidth(int unicode)
{
    return i_frGetCharWidth(unicode);
}

int prwuiGetWindowWidth()
{
    return m_windowWidth;
}

int prwuiGetWindowHeight()
{
    return m_windowHeight;
}

float prwuiGetUIwidth()
{
    return m_windowWidth / m_UIscaleFctor;
}

float prwuiGetUIheight()
{
    return m_windowHeight / m_UIscaleFctor;
}

float prwuiGetUIScaleFactor()
{
    return m_UIscaleFctor;
}

void prwuiPushStack()
{
    prwmbPushMatrix(m_meshBuilder);
}

void prwuiScale(float x, float y)
{
    vec3 v = { x, y, 1 };

    glm_scale(prwmbGetModelView(m_meshBuilder), v);
}

void prwuiTranslate(float x, float y)
{
    vec3 v = { x, y, 0 };
    glm_translate(prwmbGetModelView(m_meshBuilder), v);
}

void prwuiRotate(float x, float y, float z)
{
    vec4* modelView = prwmbGetModelView(m_meshBuilder);

    vec3 axis = { 1, 0, 0 };
    glm_rotate(modelView, glm_rad(x), axis);
    axis[0] = 0; axis[1] = 1;
    glm_rotate(modelView, glm_rad(y), axis);
    axis[1] = 0; axis[2] = 1;
    glm_rotate(modelView, glm_rad(z), axis);
}

void prwuiMult(mat4 val)
{
    vec4* modelView = prwmbGetModelView(m_meshBuilder);

    glm_mat4_mul(modelView, val, modelView);
}

void prwuiPopStack()
{
    prwmbPopMatrix(m_meshBuilder);
}

vec4* prwuiGetModelView()
{
    return prwmbGetModelView(m_meshBuilder);
}

static void i_init()
{
    glfwMakeContextCurrent(m_window);

#ifndef EMSCRIPTEN
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
#endif

    prwvfVTXFMT vertexFormat;

    vertexFormat[0] = 4; // Number of attributes

    vertexFormat[1] = PRWVF_ATTRB_USAGE_POS 
                    | PRWVF_ATTRB_TYPE_FLOAT
                    | PRWVF_ATTRB_SIZE(3)
                    | PRWVF_ATTRB_NORMALIZED_FALSE;

    vertexFormat[2] = PRWVF_ATTRB_USAGE_UV
                    | PRWVF_ATTRB_TYPE_FLOAT
                    | PRWVF_ATTRB_SIZE(2)
                    | PRWVF_ATTRB_NORMALIZED_FALSE;
                
    vertexFormat[3] = PRWVF_ATTRB_USAGE_COLOR 
                    | PRWVF_ATTRB_TYPE_FLOAT
                    | PRWVF_ATTRB_SIZE(4)
                    | PRWVF_ATTRB_NORMALIZED_FALSE;

    vertexFormat[4] = PRWVF_ATTRB_USAGE_TEXID
                    | PRWVF_ATTRB_TYPE_UBYTE
                    | PRWVF_ATTRB_SIZE(1)
                    | PRWVF_ATTRB_NORMALIZED_FALSE;
    
    m_meshBuilder = prwmbGenBuilder(vertexFormat);

    i_frInit();
}

static void i_onWindowResize(GLFWwindow* window, int width, int height)
{
    m_windowWidth = width;
    m_windowHeight = height;
}

static inline void i_orient(float* dst, int mode, float left, float top, float right, float bottom)
{
    switch(mode)
    {
    case PRWUI_TO_LEFT:
		dst[0] = left;
		dst[1] = top;
		dst[2] = left;
		dst[3] = bottom;
		dst[4] = right;
		dst[5] = bottom;
		dst[6] = right;
		dst[7] = top;
		return;
	case PRWUI_TO_RIGHT:
		dst[0] = right;
		dst[1] = bottom;
		dst[2] = right;
		dst[3] = top;
		dst[4] = left;
		dst[5] = top;
		dst[6] = left;
		dst[7] = bottom;
		return;
	case PRWUI_TO_TOP:
		dst[0] = right;
		dst[1] = top;
		dst[2] = left;
		dst[3] = top;
		dst[4] = left;
		dst[5] = bottom;
		dst[6] = right;
		dst[7] = bottom;
		return;
    default:
        dst[0] = left;
		dst[1] = bottom;
		dst[2] = right;
		dst[3] = bottom;
		dst[4] = right;
		dst[5] = top;
		dst[6] = left;
		dst[7] = top;
		return;
    }
}

//------------------------------------ ///  FONT RENDERER  /// --------------------------------------//

static const int m_ASCII_INDECIES[] = 
{
    0x00C0, 0x00C1, 0x00C2, 0x00C8, 0x00CA, 0x00CB, 0x00CD, 0x00D3, 
    0x00D4, 0x00D5, 0x00DA, 0x00DF, 0x00E3, 0x00F5, 0x011F, 0x0130, 
    0x0131, 0x0152, 0x0153, 0x015E, 0x015F, 0x0174, 0x0175, 0x017E, 
    0x0207, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F, 
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F, 
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
    0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F, 
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
    0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x0000, 
    0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7, 
    0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5, 
    0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, 
    0x00FF, 0x00D6, 0x00DC, 0x00F8, 0x00A3, 0x00D8, 0x00D7, 0x0192, 
    0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA, 
    0x00BF, 0x00AE, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB, 
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 
    0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510, 
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 
    0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567, 
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 
    0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580, 
    0x03B1, 0x03B2, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x03BC, 0x03C4, 
    0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x2205, 0x2208, 0x2229, 
    0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248, 
    0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x0000, 
};

static const char* m_FORMATTING_KEYS = "0123456789abcdefklmnors";

static const uint32_t m_COLOR_CODES[] = 
{
    -16777216, -16777046, -16733696, -16733526, -5636096, -5635926, -22016, -5592406,
    -11184811, -11184641, -11141291, -11141121,   -43691,   -43521,   -171,       -1
};

static uint32_t m_fontColor = -1;
static bool m_randomStyle = false;
static bool m_boldStyle = false;
static bool m_strikethroughStyle = false;
static bool m_underlineStyle = false;
static bool m_italicStyle = false;
static bool m_hiddenStyle = false;

static int m_maxTexUnits = 0;
static GLuint m_asciiAtlasTex = 0;
static uint8_t m_asciiGlyphWidths[256];

static inline int i_frGetCharID(int unicode)
{
    if(unicode < 256)
    {
        for(int i = 0; i < sizeof(m_ASCII_INDECIES); i++)
        {
            if(m_ASCII_INDECIES[i] == unicode) return i;
        }
    }
    return 0;
}

static void i_readAsciiWidths(stbi_uc* image, int width, int height)
{
    int xGlpyhs = width / 16;
    int yGlpyhs = height / 16;

    for(int chr = 0; chr < 256; chr++)
    {
        int glyphX = (chr % 16) * 8;
        int glyphY = (chr / 16) * 8;

        if(chr == 32)
        {
            m_asciiGlyphWidths[chr] = 4;
            continue;
        }

        for(int x = glyphX; x < glyphX + 8; x++)
        {
            int hasPixel = 0;
            for(int y = glyphY; y < glyphY + 8; y++)
            {
                int pixel = ((int*) image)[y * width + x];
                if(pixel) 
                {
                    hasPixel = 1;
                    break;
                }
            }
            if(hasPixel) m_asciiGlyphWidths[chr]++;
            else break;
        }
    }
}

static void i_frloadTexture(const char* fileName, GLuint* texId)
{
    int x, y, channels;
    stbi_uc* image = stbi_load(fileName, &x, &y, &channels, STBI_rgb_alpha);

    if(!image)
    {
        printf("[OpenGL Texture][Error]: Failed to load \"%s\"\n", fileName);
        return;
    }
    else if(strcmp(fileName, "res/ascii.png") == 0) i_readAsciiWidths(image, x, y);

    glGenTextures(1, texId);

    glBindTexture(GL_TEXTURE_2D, *texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(image);
}

static void i_frInit()
{
    if(!m_asciiAtlasTex)
    {
        i_frloadTexture("res/ascii.png", &m_asciiAtlasTex);
    }

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m_maxTexUnits);
    m_maxTexUnits = m_maxTexUnits <= 16 ? m_maxTexUnits : 16;
}

static void i_frGenChar(int unicode, float x , float y, float italics, uint32_t color)
{
    if(256 <= unicode)
    {
        return;
    }

    uint8_t glyphID = i_frGetCharID(unicode);
    uint8_t glpyhWidth = m_asciiGlyphWidths[glyphID];
    float uStart = (glyphID % 16) * 8;
    float vStart = (glyphID / 16) * 8 - 0.01;
    float uiScale = prwuiGetUIScaleFactor();
    
    vec4 colorv4 = getVec4Color(color);

    prwmbIndex(m_meshBuilder, 6, 0, 1, 3, 2, 3, 1);

    prwmbVertex(m_meshBuilder,              x - italics,  y + 8, 0.f, (uStart             ) / 128.0f, (vStart + 8) / 128.0f, vec4Color(colorv4), m_maxTexUnits);
    prwmbVertex(m_meshBuilder, x + glpyhWidth - italics,  y + 8, 0.f, (uStart + glpyhWidth) / 128.0f, (vStart + 8) / 128.0f, vec4Color(colorv4), m_maxTexUnits);
    prwmbVertex(m_meshBuilder, x + glpyhWidth + italics,      y, 0.f, (uStart + glpyhWidth) / 128.0f, (vStart    ) / 128.0f, vec4Color(colorv4), m_maxTexUnits);
    prwmbVertex(m_meshBuilder,              x + italics,      y, 0.f, (uStart             ) / 128.0f, (vStart    ) / 128.0f, vec4Color(colorv4), m_maxTexUnits);
}

static void i_frBindAtlases()
{
    glActiveTexture(GL_TEXTURE0 + m_maxTexUnits - 1);
    glBindTexture(GL_TEXTURE_2D, m_asciiAtlasTex);
    glActiveTexture(GL_TEXTURE0);
}

static inline uint8_t i_frFindRandChar(uint8_t glyphWidth)
{
    uint8_t randChar;
    uint8_t randGlyphWidth = 0;
    while(randGlyphWidth != glyphWidth)
    {
        randChar = rand();
        randGlyphWidth = m_asciiGlyphWidths[i_frGetCharID(randChar)];
    }

    return randChar;
}

static inline uint32_t i_frMixColors(uint32_t color)
{
    vec4 color1 = { color >> 16 & 255, color >> 8 & 255, color & 255, color >> 24 & 255 };
    vec4 color2 = { m_fontColor >> 16 & 255, m_fontColor >> 8 & 255, m_fontColor & 255, m_fontColor >> 24 & 255 };
    glm_vec4_mul(color1, color2, color1);

    uint32_t result = ((uint32_t) (color1[3] / 255.0f)) << 24;
    result |= (((uint32_t) (color1[0] / 255.0f)) << 16);
    result |= (((uint32_t) (color1[1] / 255.0f)) <<  8);
    result |= (((uint32_t) (color1[2] / 255.0f))      );

    return result;
}

int prwfrUnicodeFromUTF8(const unsigned char* str, int* bytesRead)
{
    int unicode = 0;

    if(*str < 128)
    {
        *bytesRead = 1;
        return *str;
    }
    else if((*str & 224) == 192) *bytesRead = 2;
    else if((*str & 240) == 224) *bytesRead = 3;
    else if((*str & 248) == 240) *bytesRead = 4;

    switch(*bytesRead)
    {
    case 2:
        unicode |= ((int) str[0] & 31) << 6;
        unicode |= str[1] & 63;
        break;
    case 3:
        unicode |= ((int) str[0] & 15) << 12;
        unicode |= ((int) str[1] & 63) << 6;
        unicode |= str[2] & 63;
        break;
    case 4:
        unicode |= ((int) str[0] &  8) << 18;
        unicode |= ((int) str[1] & 63) << 12;
        unicode |= ((int) str[2] & 63) << 6;
        unicode |= str[3] & 63;
    }

    return unicode;
}

int prwfrSplitToFitr(char* dst, const char* src, float maxWidth, const char* regex)
{
    m_boldStyle = false;
    m_hiddenStyle = false;

    int strLen = strlen(src);
    float cursor = 0.0f;

    int i;
    int regexLen = strlen(regex);
    int regexCount = 0;
    int lstRgx = 0;
    for(i = 0; i < strLen;)
    {
        int bytesRead = 0;
        int unicode = prwfrUnicodeFromUTF8(&src[i], &bytesRead);

        if(0 < regexLen && strncmp(&src[i], regex, regexLen) == 0)
        {
            regexCount++;
            lstRgx = i + regexLen;
        }

        if(unicode == 167 && i + bytesRead < strLen)
        {
            char key[] = { tolower(src[i + bytesRead]), 0 };
            int formatKey = strstr(m_FORMATTING_KEYS, key) - m_FORMATTING_KEYS;

            if(formatKey < 16 || formatKey == 21)
            {
                m_boldStyle = false;
                m_hiddenStyle = false;
            }
            else if(formatKey == 17) m_boldStyle = true;
            else if(formatKey == 22) m_hiddenStyle = true;

            i++;
        }
        else if(!m_hiddenStyle)
        {
            uint8_t glyphID = i_frGetCharID(unicode);
            uint8_t glyphWidth = m_asciiGlyphWidths[glyphID];
    
            float advance = glyphWidth + (m_boldStyle ? 2 : 1);
    
            cursor += advance;

            if(maxWidth < cursor)
            {
                int numRead = i;
                if(0 < regexLen && 0 < regexCount) numRead = lstRgx;

                strncpy(dst, src, numRead);
                dst[numRead] = '\0';
                return numRead;
            }
        }
        i += bytesRead;
    }

    strcpy(dst, src);
    return i;
}

int prwfrSplitToFit(char* dst, const char* src, float maxWidth)
{
    return prwfrSplitToFitr(dst, src, maxWidth, "");
}

void prwfrGetFormats(char* dst, const char* src)
{
    int strLen = strlen(src);
    int formatLen = 0;

    for(int i = 0; i < strLen;)
    {
        int bytesRead = 0;
        int unicode = prwfrUnicodeFromUTF8(&src[i], &bytesRead);

        if(unicode == 167 && i + bytesRead < strLen)
        {
            char key[] = { tolower(src[i + bytesRead]), 0 };
            int formatKey = strstr(m_FORMATTING_KEYS, key) - m_FORMATTING_KEYS;

            if(0 <= formatKey && formatKey <= 22) //only copy known formats
            {
                strncat(dst, src + i, 3);
                formatLen += 3;
            }
            i++;
        }
        i += bytesRead;
    }
    dst[formatLen] = '\0';
}

static inline float i_frGetCharWidth(int unicode)
{
    uint8_t glyphID = i_frGetCharID(unicode);
    return  m_asciiGlyphWidths[glyphID] + 1;
}

static inline float i_frGetStringWidth(const char* str)
{
    m_boldStyle = false;
    m_hiddenStyle = false;

    int strLen = strlen(str);
    float cursor = 0.0f;

    for(int i = 0; i < strLen;)
    {
        int bytesRead = 0;
        int unicode = prwfrUnicodeFromUTF8(&str[i], &bytesRead);

        if(unicode == 167 && i + bytesRead < strLen)
        {
            char key[] = { tolower(str[i + bytesRead]), 0 };
            int formatKey = strstr(m_FORMATTING_KEYS, key) - m_FORMATTING_KEYS;

            if(formatKey < 16 || formatKey == 21) 
            {
                m_boldStyle = false;
                m_hiddenStyle = false;
            }
            else if(formatKey == 17) m_boldStyle = true;
            else if(formatKey == 22) m_hiddenStyle = true;

            i++;
        }
        else if(!m_hiddenStyle)
        {
            uint8_t glyphID = i_frGetCharID(unicode);
            uint8_t glyphWidth = m_asciiGlyphWidths[glyphID];
    
            float advance = glyphWidth + (m_boldStyle ? 2 : 1);
    
            cursor += advance;
        }
        i += bytesRead;
    }

    return cursor;
}

static inline void i_frGenString(const char* str, float x, float y, uint32_t color)
{
    m_randomStyle = false;
    m_boldStyle = false;
    m_strikethroughStyle = false;
    m_underlineStyle = false;
    m_italicStyle = false;
    m_hiddenStyle = false;
    m_fontColor = color;

    uint32_t charColor = m_fontColor;
    int strLen = strlen(str);
    float cursor = x;

    for(int i = 0; i < strLen;)
    {
        int bytesRead = 0;
        int unicode = prwfrUnicodeFromUTF8(&str[i], &bytesRead);

        if(unicode == 167 && i + bytesRead < strLen)
        {
            char key[] = { tolower(str[i + bytesRead]), 0 };
            int formatKey = strstr(m_FORMATTING_KEYS, key) - m_FORMATTING_KEYS;

            if(formatKey < 16)
            {
                m_randomStyle = false;
                m_boldStyle = false;
                m_strikethroughStyle = false;
                m_underlineStyle = false;
                m_italicStyle = false;
                m_hiddenStyle = false;

                if(formatKey < 0) formatKey = 15;

                charColor = m_COLOR_CODES[formatKey];
            }
            else
            {
                switch(formatKey)
                {
                case 16:
                    m_randomStyle = true;
                    break;
                case 17:
                    m_boldStyle = true;
                    break;
                case 18:
                    m_strikethroughStyle = true;
                    break;
                case 19:
                    m_underlineStyle = true;
                    break;
                case 20:
                    m_italicStyle = true;
                    break;
                case 21:
                    m_randomStyle = false;
                    m_boldStyle = false;
                    m_strikethroughStyle = false;
                    m_underlineStyle = false;
                    m_italicStyle = false;
                    m_hiddenStyle = false;
                    charColor = m_fontColor;
                    break;
                case 22:
                    m_hiddenStyle = true;
                }
            }
            i++;
        }
        else if(!m_hiddenStyle)
        {
            uint8_t glyphID = i_frGetCharID(unicode);
            uint8_t glyphWidth = m_asciiGlyphWidths[glyphID];
    
            uint32_t mixedColor = i_frMixColors(charColor);
            int chr = m_randomStyle ? i_frFindRandChar(glyphWidth) : unicode;
            float advance = glyphWidth + (m_boldStyle ? 2 : 1);
            float italics = m_italicStyle ? 1 : 0;
    
            if(unicode != 32) i_frGenChar(chr, cursor, y, italics, mixedColor);
            if(m_boldStyle) i_frGenChar(chr, cursor + 1, y, italics, mixedColor);
            if(m_strikethroughStyle) prwuiGenHorizontalLine(y + 3, cursor, cursor + advance, mixedColor);
            if(m_underlineStyle) prwuiGenHorizontalLine(y + 8, cursor - 1, cursor + advance, mixedColor);
    
            cursor += advance;
        }
        i += bytesRead;
    }
}

static inline void i_frAnchor(int mode, const char* str, float x, float y, float* newpos)
{
    float stringWidth = i_frGetStringWidth(str);
    float stringHeight = 8;

    switch(mode)
    {
    case PRWUI_MID_LEFT:
    case PRWUI_CENTER:
    case PRWUI_MID_RIGHT:
        newpos[1] = y - stringHeight / 2.0f;
        break;
    case PRWUI_BOTTOM_LEFT:
    case PRWUI_BOTTOM_CENTER:
    case PRWUI_BOTTOM_RIGHT:
        newpos[1] = y -stringHeight;
        break;
    default:
        newpos[1] = y;
    }

    switch(mode)
    {
        case PRWUI_TOP_CENTER:
        case PRWUI_CENTER:
        case PRWUI_BOTTOM_CENTER:
            newpos[0] = x - stringWidth / 2.0f;
            break;
        case PRWUI_TOP_RIGHT:
        case PRWUI_MID_RIGHT:
        case PRWUI_BOTTOM_RIGHT:
            newpos[0] = x - stringWidth;
            break;
        default:
            newpos[0] = x;
    }
}