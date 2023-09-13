#ifndef SBUF_H
#define SBUF_H
#include "csapp.h"

#define SBUFSIZE 64 


typedef struct{
    int *buf;		// buffer array
    int n;			// max num of slots
    int front;		// buf[(front+1)%n] is first item
    int rear;		// buf[rear%n] is last item
    sem_t mutex;	// protect access to buf
    sem_t slots;	// count available slots
    sem_t items;	// count available items
    int size;
}sbuf_t;

void sbuf_init(sbuf_t *sp, int n);		// 初始化buffer
void sbuf_deinit(sbuf_t *sp);			// 释放buffer
void sbuf_insert(sbuf_t *sp, int item);	// 向buffer的slot插入item
int sbuf_remove(sbuf_t *sp);			// 移除item
int sbuf_full(sbuf_t *sp);
int sbuf_empty(sbuf_t *sp);
// 这里buffer是一个队列 FIFO

#endif