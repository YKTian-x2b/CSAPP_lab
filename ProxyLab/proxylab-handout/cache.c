#include "cache.h"

static volatile int totalContentSize = 0;
static volatile int nwOpIdx = 1;
static volatile int maxCacheLineIdx = 0;

static CacheLine cacheLines[MAX_CACHE_LINE_SIZE];

int findMinInvalid();
int LRU();
void realWrite(char *pth, char *buf, int bufLen, int CLI);

void readCache(char *key, char **res_ptr){
    int i = 0;
    readerLock();
    for(; i < maxCacheLineIdx; i++){
        if(cacheLines[i].valid && !strcmp(key, cacheLines[i].path)){
            printf("this way\n");
            *res_ptr = (char*)Malloc(cacheLines[i].contentLen);
            memcpy(*res_ptr, cacheLines[i].content, cacheLines[i].contentLen);
            break;
        }  
    }
    // 读写锁转换的时候，无法保证获得写锁时，cacheLines[i]还是原来的状态，同样无法保证nwOpIdx是read发生时的顺序
    // readerUnlock();
    // writerLock();
    readerToWriter();
    if(i < maxCacheLineIdx)
        cacheLines[i].RUI = nwOpIdx++;
    writerUnlock();
    return ;
}



void writeCache(char *pth, char *buf, int bufLen){
    int cached = 0;
    int i = 0;
    readerLock();
    if(maxCacheLineIdx >= MAX_CACHE_LINE_SIZE){
        fprintf(stderr, "App error: inited cacheLineArraySize is too small!");
        exit(1);
    }
    for(; i < maxCacheLineIdx; i++){
        if(cacheLines[i].valid && !strcmp(pth, cacheLines[i].path)){
            cached = 1;
            break;
        }
    }
    // readerUnlock();
    // writerLock();
    readerToWriter();
    if(cached){
        cacheLines[i].RUI = nwOpIdx++;
        writerUnlock();
        return ;
    }
    // 这里应该用链表实现
    int minInvalidCacheLineIdx = findMinInvalid();
    // 缓存未满的情况下
    if(totalContentSize + bufLen <= MAX_CACHE_SIZE){
        printf("totalContentSize + bufLen <= MAX_CACHE_SIZE\n");
        // 如果没有invalid的cacheline，则继续新增cacheline
        if(minInvalidCacheLineIdx == ~(1<<31)){
            printf("maxCacheLineIdx\n");
            realWrite(pth, buf, bufLen, maxCacheLineIdx);
        }
        else{
            // 如果有invalid的cacheline，则选个最小的
            printf("minInvalidCacheLineIdx: %d\n", minInvalidCacheLineIdx);
            realWrite(pth, buf, bufLen, minInvalidCacheLineIdx);
        }
    }
    else{
        printf("totalContentSize + bufLen > MAX_CACHE_SIZE\n");
        // 想写入一个buf就得腾出来buflen的空间，这可能需要多个cacheline的换出
        // 类LRU，还是最LRU的最先换出，只不过，可能需要换出多个
        while(totalContentSize + bufLen > MAX_CACHE_SIZE){
            int nwLRUI = LRU();
            // 理论上来说 这不太会发生 因为 totalContentSize < MAX_CACHE_SIZE 的情况包含了它
            if(nwLRUI == -1){
                fprintf(stderr, "The cacheLine is empty!!!");
            }
            Free(cacheLines[nwLRUI].path);
            Free(cacheLines[nwLRUI].content);
            cacheLines[nwLRUI].valid = 0;
            totalContentSize -= cacheLines[nwLRUI].contentLen;
        }
        minInvalidCacheLineIdx = findMinInvalid();
        realWrite(pth, buf, bufLen, minInvalidCacheLineIdx);
    }

    writerUnlock();
}

int findMinInvalid(){
    int minInvalidCacheLineIdx = ~(1<<31);
    for(int i = 0; i < maxCacheLineIdx; i++){
        if(!cacheLines[i].valid){
            minInvalidCacheLineIdx = minInvalidCacheLineIdx < i ? minInvalidCacheLineIdx : i;
        }
    }
    return minInvalidCacheLineIdx;
}

int LRU(){
    int minRUI = ~(1 << 31);
    int minCacheLineIdx = -1;
    for(int i = 0; i < maxCacheLineIdx; i++){
        if(cacheLines[i].valid){
            if(cacheLines[i].RUI < minRUI){
                minRUI = cacheLines[i].RUI;
                minCacheLineIdx = i;
            }
        }
    }
    return minCacheLineIdx;
}

void realWrite(char *pth, char *buf, int bufLen, int CLI){
    printf("realWrite: pth: %s", pth);
    int pth_len = strlen(pth)+1;
    cacheLines[CLI].path = (char*)Malloc(pth_len);
    memcpy(cacheLines[CLI].path, pth, pth_len);
    cacheLines[CLI].content = (char*)Malloc(bufLen);
    memcpy(cacheLines[CLI].content, buf, bufLen);
    cacheLines[CLI].contentLen = bufLen;
    cacheLines[CLI].RUI = nwOpIdx++;
    cacheLines[CLI].valid = 1;
    //
    totalContentSize += bufLen;
    if(CLI == maxCacheLineIdx){
        maxCacheLineIdx++;
    }
}