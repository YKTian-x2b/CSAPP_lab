#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include "csapp.h"
#include "sbuf.h"
#include <pthread.h>

#define INIT_THREAD_N 8
#define THREAD_LIMIT 4096

extern sbuf_t sbuf;

void initThreadPool();
void *adjustThreadPool(void*);
void deInitThreadPool();

#endif