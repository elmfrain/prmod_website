#include "markdown_viewer.h"

#include "ui_render.h"
#include "https_fetcher.h"
#include "animation.h"
#include "input.h"

#ifndef EMSCRIPTEN
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#include <emscripten.h>
#endif
#include <stb_image.h>

#include <stdio.h>
#include <string.h>

//-------- LIST CONVINIENCE CLASS --------//
struct ListNode
{
    void* object;
    void* p_next;
} ListNode;
typedef struct ListNode node_t;
struct List
{
    node_t* head;
    node_t* end;
    node_t* get;
    int getPos;
    int size;
    void (*destructor)(void*);
} List;
typedef struct List list_t;

static void i_listDefaultDestructor(void* object)
{
    free(object);
}

static list_t i_listCreate()
{
    list_t list;
    list.head = NULL;
    list.end = NULL;
    list.get = NULL;
    list.getPos = 0;
    list.size = 0;
    list.destructor = i_listDefaultDestructor;
    return list;
}
static void i_listAdd(list_t* list, void* object)
{
    node_t* newNode = malloc(sizeof(node_t));

    if(!list->size)
    {
        list->head = newNode;
        list->end = newNode;
    }
    list->end->p_next = newNode;
    list->end = newNode;

    newNode->object = object;
    newNode->p_next = NULL;
    list->size++;
}
static void* i_listGet(list_t* list, int index)
{
    if(!list->size) return NULL;

    if(!list->get || list->getPos < index)
    {
        list->get = list->head;
        list->getPos = 0;
    }
    for(; list->getPos < list->size; list->getPos++)
    {
        if(index == list->getPos) return list->get->object;
        list->get = list->get->p_next;
    }

    return NULL;
}
static void i_listClear(list_t* list)
{
    if(!list->size) return;

    node_t* node = list->head;
    for(int i = 0; i < list->size; i++)
    {
        if(node->object) list->destructor(node->object);
        node_t* toDelete = node;
        node = node->p_next;
        free(toDelete);
    }

    list->head = NULL;
    list->end = NULL;
    list->get = NULL;
    list->size = 0;
}

//------- MARKDOWN VIEWER --------//

struct MarkdownViewer
{
    PRWwidget* widget;
    PRWfetcher* fetcher;
    PRWsmoother scroll;
    list_t imageList;
    list_t linkList;
} MarkdownViewer;

static struct MDimage
{
    PRWfetcher* fetcher;
    GLuint glTexture;
    int width, height;
    char url[2048];
    char alt[256];
} MDimage;

static struct MDlink
{
    int linkPos;
    PRWwidget* button;
    char url[2048];
    char alt[256];
} MDlink;

static struct MarkdownViewer* currentMDviewer = NULL;

static void i_onLinkClick(PRWwidget* widget)
{
    for(int i = 0; i < currentMDviewer->linkList.size; i++)
    {
        struct MDlink* l = i_listGet(&currentMDviewer->linkList, i);
        if(l->button == widget)
        {
            int ret;
            char command[3000];
#ifdef EMSCRIPTEN
            //printf("I got this link!: %s", l->url);
            sprintf(command, "window.open(\"%s\");", l->url);
            emscripten_run_script(command);
#elif defined(__unix__)
            sprintf(command, "xdg-open %s", l->url);
            ret = system(command);
#elif defined(_WIN32)
            sprintf(command, "start %s", l->url);
            ret = system(command);
#elif defined(__APPLE__)
            sprintf(command, "open %s", l->url);
            ret = system(command);
#endif
        }
    }
}

static void i_drawLinks(struct MarkdownViewer* v)
{
    currentMDviewer = v;
    for(int i = 0; i < v->linkList.size; i++)
    {
        struct MDlink* l = i_listGet(&v->linkList, i);
        prwwWidgetDraw(l->button);
    }
}

static struct MDlink* i_getLinkFromPos(struct MarkdownViewer* v, int pos)
{
    for(int i = 0; i < v->linkList.size; i++)
    {
        struct MDlink* l = i_listGet(&v->linkList, i);
        if(l->linkPos == pos) return l;
    }

    return NULL;
}

static struct MDlink* i_getLink(struct MarkdownViewer* v, const char* url, const char* alt)
{
    for(int i = 0; i < v->linkList.size; i++)
    {
        struct MDlink* l = i_listGet(&v->linkList, i);
        if(strstr(url, l->url)) return l;
    }

    struct MDlink newLink;

    newLink.linkPos = -1;
    newLink.button = prwwGenWidget(PRWW_TYPE_WIDGET);
    newLink.button->width = prwuiGetStringWidth(alt);
    newLink.button->height = prwuiGetStringHeight();
    prwwWidgetSetCallback(newLink.button, i_onLinkClick);
    strcpy(newLink.url, url);
    strcpy(newLink.alt, alt);
    struct MDlink* l = malloc(sizeof(struct MDlink));
    *l = newLink;
    i_listAdd(&v->linkList, l);

    return l;
}

static struct MDimage* i_getImage(struct MarkdownViewer* v, const char* url, const char* alt)
{
    for(int i = 0; i < v->imageList.size; i++)
    {
        struct MDimage* im = i_listGet(&v->imageList, i);
        if(im && strstr(url, im->url))
        {
            if(!im->glTexture && im->fetcher && prwfFetchComplete(im->fetcher))
            {
                int channels, len;
                stbi_set_flip_vertically_on_load(1);
                const char* imgf = prwfFetchString(im->fetcher, &len);
                stbi_uc* image = stbi_load_from_memory(imgf, len, &im->width, &im->height, &channels, STBI_rgb_alpha);

                if(!image)
                {
                    printf("[Markdown][Error]: Failed to load texture: \"%s\" for %s\n", im->alt, stbi_failure_reason());
                    return im;
                }
                glGenTextures(1, &im->glTexture);

                glBindTexture(GL_TEXTURE_2D, im->glTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, im->width, im->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
                glGenerateMipmap(GL_TEXTURE_2D);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glBindTexture(GL_TEXTURE_2D, 0);

                stbi_image_free(image);
            }
            return im;
        }
    }

    struct MDimage newImage;
    newImage.fetcher = prwfFetchURL(url);
    newImage.glTexture = 0;
    newImage.width = 0;
    newImage.height = 0;
    strcpy(newImage.url, url);
    strcpy(newImage.alt, alt);
    struct MDimage* im = malloc(sizeof(struct MDimage));
    *im = newImage;
    i_listAdd(&v->imageList, im);

    return im;
}

//String insert
static char strbuffer[2048];
#define strins(dst, src, rep) {strcpy(strbuffer, dst + rep); strcpy(dst, src); strcat(dst, strbuffer);}

#define getviewer struct MarkdownViewer* v = (struct MarkdownViewer*) viewer

void prwmdDrawMarkdown(PRWmarkdownViewer* viewer)
{
    getviewer;

    prwwViewportStart(v->widget, 1);
    prwuiTranslate(0, 10);

    int imgIndex = 0;
    int len;
    int lineLen = 0;

    float width = v->widget->width;
    float height = v->widget->height;
    float cursorY = 0.0f;
    float scroll = prwaSmootherValue(&v->scroll);
    const char* markdown = prwfFetchString(v->fetcher, &len);
    for(int i = 0; i < len; i++, lineLen++)
    {
        //New line found
        if(markdown[i] == '\n' || markdown[i] == '\0')
        {
            //Line states and strings
            char line[2048] = {0};
            float scale = 1.0f;
            float cursorX = 0.0f;
            char bulletPoint = 0;
            char quote = 0;
            char bold = 0;
            char code = 0;
            struct MDimage* image = NULL;
            strncpy(line, &markdown[i] - lineLen, lineLen < sizeof(line) - 1 ? lineLen : sizeof(line) - 1);

            char* res;
            if(strncmp(line, "### ", 4) == 0) //Format H3
            {
                strins(line, "§e§l", 4);
                scale = 1.25f;
            }

            else if(strncmp(line, "## ", 3) == 0) //Format H2
            {
                strins(line, "§e§l", 3);
                scale = 1.5f;
            }
            else if(strncmp(line, "* ", 2) == 0) //Add Bullet point
            {
                strins(line, "", 2);
                cursorX = 10;
                bulletPoint = 1;
            }
            else if(strncmp(line, ">", 1) == 0) //Format Quote
            {
                strins(line, "§7", 1);
                cursorX = 10;
                quote = 1;
            }

            if((res = strstr(line, "!["))) //Image Found
            {
                //Format image alt
                strins(res, "§9", 2);

                //Get image name; alt
                int j;
                for(j = 0; res[j] != ']'; j++);
                char alt[256] = {0}; strncpy(alt, res, j);

                //Get image url
                if((res = strstr(res, "(")))
                {
                    res++;
                    for(j = 0; res[j] != ')'; j++);
                    char url[2048] = {0}; strncpy(url, res, j);
                    if(*alt) image = i_getImage(v, url, alt);
                }
            }

            while((res = strstr(line, "**"))) //Format Bold
            {
                bold = !bold;
                strins(res, bold ? "§a§l" : "§r", 2);
            }

            while((res = strstr(line, "`"))) //Format Code
            {
                code = !code;
                strins(res, code ? "§7" : "§r", 1);
            }

            int lpos = 0;
            while((res = strstr(line, "["))) //Link Found
            {
                //Format link
                char* linkStart = res;
                strins(res, "§s\1§9§n", 1);

                //Copy link name
                int j;
                for(j = 0; res[j] != ']'; j++);
                char alt[256] = {0}; strncpy(alt, res + 10, j - 10);

                //Hide url
                res = strstr(res, "]");
                if(res) strins(res, "§s", 1);

                //Copy link url
                if((res = strstr(res, "(")))
                {
                    res++;
                    for(j = 0; res[j] != ')'; j++);
                    char url[2048] = {0}; strncpy(url, res, j);
                    if(*alt) 
                    {
                        struct MDlink* link = i_getLink(v, url, alt);
                        link->linkPos = lpos++;
                    }
                }

                res = strstr(res, ")");
                if(res) strins(res + 1, "§r", 0);
            }

            lineLen = strlen(line);

            //Replace line to image alt
            if(image) strcpy(line, image->alt);

            float lineHeight = 10;
            float uiScale = prwuiGetUIScaleFactor();
            scroll = (int) (scroll * uiScale);
            scroll /= uiScale;

            int linePos = 0;
            char subline[4096] = {0};
            char f[512] = {0};

        subLine:
            //Split line if it's longer than the width
            prwfrGetFormats(f + strlen(f), subline);
            linePos += prwfrSplitToFitr(subline, line + linePos, width - cursorX, " ");
            strins(subline, f, 0);

            //Position link buttons
            res = subline;
            lpos = 0;
            while((res = strstr(res, "\1")))
            {
                res++;
                struct MDlink* link = i_getLinkFromPos(v, lpos++);
                if(link)
                {
                    char isostr[1024] = {0};
                    strncpy(isostr, subline, res - subline);
                    link->button->x = cursorX + prwuiGetStringWidth(isostr);
                    link->button->y = cursorY;
                }
            }

            //Draw texts or images
            prwuiPushStack();
            {
                prwuiTranslate(0, cursorY - scroll);
                prwuiScale(scale, scale);

                if(image && image->glTexture)
                {
                    glActiveTexture(GL_TEXTURE0 + imgIndex);
                    glBindTexture(GL_TEXTURE_2D, image->glTexture);

                    float imWidth = image->width / uiScale;
                    float imHeight = image->height / uiScale;
                    if(width < imWidth + cursorX)
                    {
                        float imScale = width / (imWidth + cursorX * uiScale);
                        imWidth *= imScale;
                        imHeight *= imScale;
                    }

                    prwuiGenQuad(cursorX, 0, cursorX + imWidth, imHeight, -1, imgIndex++ + 1);
                    lineHeight = (int) imHeight;
                }
                else
                {
                    prwuiGenString(PRWUI_TOP_LEFT, subline, cursorX, 0, -1);
                    lineHeight = 10 * scale;
                }

                cursorY += lineHeight;

                if(bulletPoint) 
                {
                    prwuiGenQuad(0, 2, 3, 5, -5592406, 0);
                    bulletPoint = 0;
                }
                if(quote) prwuiGenQuad(0, 0, 3, lineHeight, -865704346, 0);
            }
            prwuiPopStack();

            //If this line is cut and still has more text to show
            if(!image && linePos < lineLen) goto subLine;

            lineLen = -1;
        }
    }

    //Draw link buttons
    prwuiPushStack();
    {
        prwuiTranslate(0, -scroll);
        i_drawLinks(v);
    }
    prwuiPopStack();

    //Handle and constrain scroll
    if(prwiJustScrolled() && prwwWidgetHovered(v->widget)) v->scroll.grabbingTo += prwiScrollDeltaY() * 35;
    float MAX_SCROLL = cursorY - height * 2 / 3;
    if(MAX_SCROLL < 0) MAX_SCROLL = 0;
    if(scroll < 0) prwaSmootherGrabTo(&v->scroll, 0);
    else if(scroll > MAX_SCROLL) prwaSmootherGrabTo(&v->scroll, MAX_SCROLL);

    prwwViewportEnd(v->widget);
}

static void i_mdImageDestructor(void* mdImage)
{
    struct MDimage* mdi = (struct MDimage*) mdImage;
    prwfFreeFetcher(mdi->fetcher);
    if(mdi->glTexture) glDeleteTextures(1, &mdi->glTexture);
    free(mdImage);
}

static void i_mdLinkDestructor(void* mdLink)
{
    struct MDlink* mdl = (struct MDlink*) mdLink;
    prwwDeleteWidget(mdl->button);
    free(mdLink);
}

PRWmarkdownViewer* prwmdGenMarkdown(const char* url)
{
    struct MarkdownViewer* viewer = malloc(sizeof(struct MarkdownViewer));

    if(viewer)
    {
        viewer->widget = prwwGenWidget(PRWW_TYPE_VIEWPORT);
        viewer->fetcher = prwfFetchURL(url);
        prwaInitSmoother(&viewer->scroll);
        viewer->scroll.grabbed = 1;
        viewer->imageList = i_listCreate();
        viewer->linkList = i_listCreate();
        viewer->imageList.destructor = i_mdImageDestructor;
        viewer->linkList.destructor = i_mdLinkDestructor;
    }

    return (PRWmarkdownViewer*) viewer;
}

void prwmdFreeMarkdown(PRWmarkdownViewer* viewer)
{
    getviewer;
    prwwDeleteWidget(v->widget);
    prwfFreeFetcher(v->fetcher);
    i_listClear(&v->imageList);
    i_listClear(&v->linkList);
    free(v);
}