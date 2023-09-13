#include "csapp.h"

#define N 6

void *thread(void *vargp);

char **ptr;

int main(){
    int i;
    pthread_t tid;
    char *msgs[N] = {
        "Hello from foo1",
        "Hello from foo2",
        "Hello from foo3",
        "Hello from foo4",
        "Hello from foo5",
        "Hello from foo6"
    };
    ptr = msgs;
    for(i = 0; i < N; i++){
        Pthread_create(&tid, NULL, thread, (void*)i);
    }
    Pthread_exit(NULL);
}

void *thread(void * vargp){
    int myid = (int) vargp;
    static int cnt = 0;
    printf("[%d]: %s (cnt=%d)\n", myid, ptr[myid], ++cnt);
    return NULL;
}