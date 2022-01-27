#include "https_fetcher.h"

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

#ifndef EMSCRIPTEN
#include <openssl/ssl.h>
#endif

#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <pthread.h>

#ifdef EMSCRIPTEN
#include <emscripten/fetch.h>
#endif

static struct Fetcher
{
    char complete;
    char* rawData;
    char statusStr[64];
    char* data;
    int datalen;
    int headerlen;
    short statusCode;

    char hostname[256];
    char dir[2048];
    char ip[100];
#ifndef EMSCRIPTEN //Fetcher will use the Fetch API if compiling with emscripten
    struct sockaddr_in address;
    int connection;
    SSL* sslConn;
#endif
    pthread_t threadID;
} Fetcher;

#ifndef EMSCRIPTEN
static SSL_CTX* m_sslContext = NULL;
#endif

static void i_init();
static void i_getIPfromName(const char* hostname, char* ip);
static void* i_fetcherWorker(void* fetcher);

PRWfetcher* prwfFetch(const char* host, const char* dir)
{
    struct Fetcher* newFetcher = malloc(sizeof(struct Fetcher));
#ifndef EMSCRIPTEN
    if(!m_sslContext) i_init();
#endif
    if(newFetcher)
    {
        newFetcher->complete = 0;
        newFetcher->rawData = NULL;
        memset(newFetcher->statusStr, 0, sizeof(newFetcher->statusStr));
        newFetcher->data = NULL;
        newFetcher->datalen = 0;
        newFetcher->headerlen = 0;
        newFetcher->statusCode = 200;
        memset(newFetcher->hostname, 0, sizeof(newFetcher->hostname));
        memset(newFetcher->dir, 0, sizeof(newFetcher->dir));
        memset(newFetcher->ip, 0, sizeof(newFetcher->ip));

        strncpy(newFetcher->hostname, host, sizeof(newFetcher->hostname) - 1);
        strncpy(newFetcher->dir, dir, sizeof(newFetcher->dir) - 1);

        pthread_create(&newFetcher->threadID, NULL, i_fetcherWorker, newFetcher);
    }

    return (PRWfetcher*) newFetcher;
}

PRWfetcher* prwfFetchURL(const char* url)
{
    int urlLen = strlen(url) + 1;

    if(strstr(url, "https://") == url)
    {
        url += 8; //Skip the 'https://'
        char hostname[256] = {0};
        char dir[2048] = {0};

        int i;
        for(i = 0; i < urlLen; i++)
            if(url[i] == '/' || url[i] == '\0')
            {
                if(url[i] == '\0') dir[0] = '/';
                strncpy(hostname, url, i);
                break;
            }

        strncpy(dir, &url[i], sizeof(dir));

        return prwfFetch(hostname, dir);
    }
    else printf("[Fetcher][Error]: Url \"%s\" does not use the https protocol", url);

    return NULL;
}

int prwfFetchComplete(PRWfetcher* fetcher)
{
    struct Fetcher* f = (struct Fetcher*) fetcher;
    return f->complete;
}

//#define RAW_DATA //Enable for debugging
const char* prwfFetchString(PRWfetcher* fetcher, int* length)
{
    struct Fetcher* f = (struct Fetcher*) fetcher;
    if(!f->complete)
    {
        *length = strlen(f->statusStr) + 1;
        return f->statusStr;
    }
    #ifndef RAW_DATA
    *length = f->datalen;
    return f->data;
    #else
    *length = f->headerlen + f->datalen;
    return f->rawData;
    #endif
}

void prwfFetchWait(PRWfetcher* fetcher)
{
    struct Fetcher* f = (struct Fetcher*) fetcher;
    pthread_join(f->threadID, NULL);
}

void prwfFreeFetcher(PRWfetcher* fetcher)
{
    struct Fetcher* f = (struct Fetcher*) fetcher;
    pthread_join(f->threadID, NULL);
#ifndef EMSCRIPTEN
    SSL_free(f->sslConn);
#endif
    free(f->rawData);
    free(fetcher);
}

static void* i_fetcherWorker(void* fetcher)
{
    struct Fetcher* f = (struct Fetcher*) fetcher;
    int ret;
#ifndef EMSCRIPTEN
    f->connection = socket(AF_INET, SOCK_STREAM, 0);
    if(f->connection < 0)
    {
        printf("[Fetcher][Error]: Unable to create socket\n");
        strcpy(f->statusStr, "Fetch Failed");
        return NULL;
    } else strcpy(f->statusStr, "Created Socket");

    f->address.sin_family = AF_INET;
    f->address.sin_port = htons(443);
    i_getIPfromName(f->hostname, f->ip);

    if(inet_pton(AF_INET, f->ip, &f->address.sin_addr) <= 0)
    {
        printf("[Fetcher][Error]: Invalid address: %s\n", f->ip);
        strcpy(f->statusStr, "Fetch Failed");
        return NULL;
    } else strcpy(f->statusStr, "Parsed Address");

    printf("[Fetcher][Info]: Connecting to %s ...\n", f->ip);

    if(connect(f->connection, (struct sockaddr*)&f->address, sizeof(f->address)) < 0)
    {
        printf("[Fetcher][Error]: Unable to connect to %s\n", f->ip);
        strcpy(f->statusStr, "Fetch Failed");
        return NULL;
    } else strcpy(f->statusStr, "Connected to Server");

    f->sslConn = SSL_new(m_sslContext);
    SSL_set_fd(f->sslConn, f->connection);
    SSL_set_connect_state(f->sslConn);
    if(ret = (SSL_connect(f->sslConn)) <= 0)
    {
        printf("[Fetcher][Error]: Unable to establish SSL to %s. Code : %d\n", f->ip, ret);
        strcpy(f->statusStr, "Fetch Failed");
        return NULL;
    } else strcpy(f->statusStr, "Established SSL");

    size_t bytesRead = 0;
    size_t rawDataPos = 0;
    size_t headerLength = 0;
    size_t contentLength = 0;
    int capacity = 0;

    char request[4098] = {0};
    sprintf(request, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", f->dir, f->hostname);

    SSL_write(f->sslConn, request, strlen(request));
    strcpy(f->statusStr, "Sent Request");

    //Get HTTP Headers
    do
    {
        capacity += 2048;
        f->rawData = realloc(f->rawData, capacity);
        SSL_read_ex(f->sslConn, f->rawData + rawDataPos, capacity - rawDataPos, &bytesRead);
        rawDataPos += bytesRead;
        f->data = strstr(f->rawData, "\r\n\r\n");
    } while (bytesRead != 0 && !f->data);
    f->data += 4;
    if(f->data) headerLength = f->data - f->rawData;
    char* res;

    if((res = strstr(f->rawData, "HTTP/1.1")))
        f->statusCode = atoi(res + 8);
    if((res = strstr(f->rawData, "Content-Length:")))
        contentLength = atoi(res + 15);

    if(headerLength + contentLength + 1 > capacity)
    {
        capacity = headerLength + contentLength + 1;
        f->rawData = realloc(f->rawData, capacity); 
        f->data = f->rawData + headerLength;
    }

    //Get content body, if any data left
    while(rawDataPos - headerLength < contentLength)
    {
        SSL_read_ex(f->sslConn, f->rawData + rawDataPos, capacity - rawDataPos, &bytesRead);
        rawDataPos += bytesRead;
    }

    f->data[contentLength] = 0; //Null terminator
    f->headerlen = headerLength;
    f->datalen = contentLength + 1;
    sprintf(f->statusStr, "Recived Data. Status %d", f->statusCode);

    close(f->connection);

    switch(f->statusCode)
    {
    case 200: //OK
        f->complete = 1;
        break;
    case 302: //FOUND: redirect
        if((res = strstr(f->rawData, "Location:")) && (res = strstr(res, "https://")))
        {
            char newURL[4098] = {0};
            int i;
            for(i = 0; res[i]; i++) if(res[i] == '\r') break;
            strncpy(newURL, res, i);

            PRWfetcher* redirect = prwfFetchURL(newURL);
            struct Fetcher* rf = (struct Fetcher*) redirect;
            prwfFetchWait(redirect);
            f->rawData = realloc(f->rawData, rf->headerlen + rf->datalen);
            memcpy(f->rawData, rf->rawData, rf->headerlen + rf->datalen);
            f->data = f->rawData + rf->headerlen;
            f->headerlen = rf->headerlen;
            f->datalen = rf->datalen;
            prwfFreeFetcher(redirect);
            f->complete = 1;
        }
        break;
    }
#else
    strcpy(f->statusStr, "Initializing Fetcher");
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;

    char url[4098] = {0};
    strcat(url, "https://");
    strcat(url, f->hostname);
    strcat(url, f->dir);

    strcpy(f->statusStr, "Sent Request");
    emscripten_fetch_t *fetch = emscripten_fetch(&attr, url);
    f->statusCode = fetch->status;
    sprintf(f->statusStr, "Recived Data. Status: %d", f->statusCode);

    switch(f->statusCode)
    {
    case 0:
        printf("[Fetcher][Error]: Unable to fetch %s. Ensure that \'Access-Control-Allow-Origin\' is enabled for that resource.\n", url);
        break;
    case 200:
        f->datalen = fetch->numBytes + 1;
        f->rawData = malloc(f->datalen);
        memcpy(f->rawData, fetch->data, fetch->numBytes);
        f->rawData[fetch->numBytes] = '\0';
        f->data = f->rawData;
        f->complete = 1;
        break;
    }

    emscripten_fetch_close(fetch);
#endif  
    return NULL;
}

#ifndef EMSCRIPTEN
static void i_init()
{
    SSL_library_init();
    SSLeay_add_ssl_algorithms();
    SSL_load_error_strings();
    m_sslContext = SSL_CTX_new(TLS_client_method());
}
#endif

static void i_getIPfromName(const char* hostname, char* ip)
{
    struct hostent* host;
    struct in_addr** addressList;

    host = gethostbyname(hostname);
    if(!host)
    {
        printf("[Fetcher][Error]: Unknown hostname! %s\n", hostname);
        *ip = 0;
        return;
    }

    addressList = (struct in_addr**) host->h_addr_list;

    for(int i = 0; addressList[i]; i++)
    {
        strcpy(ip, inet_ntoa(*addressList[i]));
    }
}