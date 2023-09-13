#include "sbuf.h"

void sbuf_init(sbuf_t *sp, int n){
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->slots, 0, n);
    Sem_init(&sp->items, 0, 0);
    sp->size = 0;
}

void sbuf_deinit(sbuf_t *sp){
    Free(sp->buf);
}

void sbuf_insert(sbuf_t *sp, int item){
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[(++sp->rear)%(sp->n)] = item;
    sp->size++;
    V(&sp->mutex);
    V(&sp->items);
}

int sbuf_remove(sbuf_t *sp){
    P(&sp->items);
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    P(&sp->mutex);
    int item = sp->buf[(++sp->front)%(sp->n)];
    sp->size--;
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}

int sbuf_empty(sbuf_t *sp){
    int res = 0;
    P(&sp->mutex);
    res = sp->size == 0;
    V(&sp->mutex);
    return res;
}

int sbuf_full(sbuf_t *sp){
    int res = 0;
    P(&sp->mutex);
    res = sp->size == sp->n;
    V(&sp->mutex);
    return res;
}