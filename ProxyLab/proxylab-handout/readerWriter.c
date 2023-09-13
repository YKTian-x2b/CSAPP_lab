#include "readerWriter.h"

sem_t mtx, w;
sem_t mtxMtx;
static int volatile read_cnt = 0;

void init_reader_writer(){
    Sem_init(&mtx, 0, 1);
    Sem_init(&w, 0, 1);
    Sem_init(&mtxMtx, 0, 1);
}

inline void readerLock(){
    P(&mtxMtx);
    P(&mtx);
    if(read_cnt == 0){
        P(&w);
    }
    read_cnt++;
    V(&mtx);
    V(&mtxMtx);
}

inline void readerUnlock(){
    P(&mtx);
    if(read_cnt == 1){
        V(&w);
    }
    read_cnt--;
    V(&mtx);
}

inline void writerLock(){
    P(&mtxMtx);
    P(&w);
}

inline void writerUnlock(){
    V(&w);
    V(&mtxMtx);
}

// 前提是P(&mtx) 这时w也一定是锁着的
// 我们 原子的 V(mtx) 保证其他线程无法 P(mtx)或P(w);
void readerToWriter(){
    // 如果readerToWriter拿到mtxMtx，那将没有新的reader和writer能拿到锁；
    P(&mtxMtx);
    // 忙等待 直到，只有当前reader在读
    // 这里应该用 conditional 来替换忙等待
    while(read_cnt > 1){
        ;
    }
    read_cnt--;
    V(&mtx);
}