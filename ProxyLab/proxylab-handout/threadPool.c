#include "threadPool.h"

static pthread_t threadPool[THREAD_LIMIT];
sbuf_t sbuf;
static int num_threads;

void doit(int fd);

void *job(void *){
    Pthread_detach(pthread_self());
    while(1){
        int connfd = sbuf_remove(&sbuf);
        doit(connfd);
        Close(connfd);
        // 在取消点结束线程 大概是阻塞的时候
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }
}

void createThreadPool(int start, int end){
    for(int i = start; i < end; i++){
        Pthread_create(&threadPool[i], NULL, job, NULL);
    }
}

void initThreadPool(){
    sbuf_init(&sbuf, SBUFSIZE);
    num_threads = INIT_THREAD_N;
    createThreadPool(0, num_threads);
}

void *adjustThreadPool(void*){
    while(1){
        int prev = num_threads;
        if(sbuf_full(&sbuf)){
            if(prev >= THREAD_LIMIT){
                fprintf(stderr, "Too many threads, could not extend scale");
                continue;
            }
            num_threads *= 2;
            createThreadPool(prev, num_threads);
        }
        else if(sbuf_empty(&sbuf)){
            if(prev <= INIT_THREAD_N){
                continue;
            }
            num_threads /= 2;
            for(int i = num_threads; i < prev; i++){
                Pthread_cancel(threadPool[i]);
            }
        }
    }
}

void deInitThreadPool(){
    sbuf_deinit(&sbuf);
}