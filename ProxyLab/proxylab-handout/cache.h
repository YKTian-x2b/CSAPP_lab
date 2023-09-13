#ifndef CACHE_H
#define CACHE_H

#include "sbuf.h"
#include "csapp.h"
#include "readerWriter.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define MAX_CACHE_LINE_SIZE 1024

typedef struct{
    char *path;
    char *content;
    int contentLen;
    int RUI;        // Recently Uesd Index
    int valid;
}CacheLine;

void readCache(char *key, char **res);
void writeCache(char *pth, char *buf, int bufLen);

#endif