#include "cachelab.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int s = 0;
int E = 0;
int b = 0;
int nw_ins_idx = 1;     // 当前指令索引，recentlyUsed用它来记录最近使用某个缓存行的指令索引，以确定组内的lru
int miss = 0;
int hit = 0;
int eviction = 0;

char OP;                // 当前指令操作

typedef struct{
    int valid;
    int tag;
    int lru;
}CacheLine;

CacheLine * cacheLineSet;       // 缓存
int setSize = 0;

typedef struct{
    int tag;
    int setIdx;
    int blockOffset;
}MemAddr;

// hex char to binary char
void addrToBinary(char * start, int len){
    char *res = (char*)malloc(64);
    for(int i = 0; i < 64; i++){
        res[i] = '0';
    }
    for(int i = 1; i <= len; i++){
        int DecimalNum = 0;
        switch (start[len-i]){
            case 'a':
                DecimalNum = 10;
                break;
            case 'b':
                DecimalNum = 11;
                break;
            case 'c':
                DecimalNum = 12;
                break;
            case 'd':
                DecimalNum = 13;
                break;
            case 'e':
                DecimalNum = 14;
                break;
            case 'f':
                DecimalNum = 15;
                break;
            default:
                DecimalNum = start[len-i] - '0';
                break;
        }
        for(int j = 0; j < 4; j++){
            res[(i-1)*4+j] = (DecimalNum%2) + '0';
            DecimalNum /= 2;
        }
    }
    // error 15->63
    for(int i = 15; i >= 0; i--){
        if((i+1) % 4 == 0 && i != 15){
            printf(",");
        }
        printf("%c", res[i]);
    }
    printf("\n");
    free(res);
}

// 组内最小指令索引对应的缓存行被换出
int LRU(MemAddr addr, int groupIdx){
    int minInsIdx = cacheLineSet[groupIdx].lru;
    int resSetIdx = groupIdx;
    for(int i = 1; i < E; i++){
        int nw_setIdx = groupIdx + i;
        if(cacheLineSet[nw_setIdx].lru < minInsIdx){
            minInsIdx = cacheLineSet[nw_setIdx].lru;
            resSetIdx = nw_setIdx;
        }
    }
    return resSetIdx;
}

void processAddr(char * start, int len){
    // addrToBinary(start, len);
    int base = 1;
    unsigned int DecimalNum = 0;
    for(int i = 1; i <= len; i++){
        int tmp = 0;
        switch (start[len-i]){
            case 'a':
                tmp = 10;
                break;
            case 'b':
                tmp = 11;
                break;
            case 'c':
                tmp = 12;
                break;
            case 'd':
                tmp = 13;
                break;
            case 'e':
                tmp = 14;
                break;
            case 'f':
                tmp = 15;
                break;
            default:
                tmp = start[len-i] - '0';
                break;
        }
        DecimalNum += base * tmp;
        base *= 16;
    }
    MemAddr addr;
    int tool1 = ~(-1 << b);
    addr.blockOffset = DecimalNum & tool1;
    DecimalNum >>= b;
    int tool2 = ~(-1 << s);
    addr.setIdx = DecimalNum & tool2;
    addr.tag = DecimalNum >> s;
    // printf("%d %d %d\n", addr.blockOffset, addr.setIdx, addr.tag);

    int hitFlag = 0;
    int missFlag = 1;
    int evictionFlag = 0;
    int groupIdx = addr.setIdx*E;
    for(int i = 0; i < E; i++){
        if(cacheLineSet[groupIdx+i].valid == 0){
            continue;
        }
        else if(cacheLineSet[groupIdx+i].tag == addr.tag){
            hitFlag = 1;
            missFlag = 0;
            cacheLineSet[groupIdx+i].lru = nw_ins_idx;
            break;
        }
        else{
            continue;
        }
    }
    // M操作的写必命中
    if(OP == 'M')           hit++;
    if(hitFlag){
        hit++;
        return;
    }
    // 如果未命中则需要更新缓存
    int victim = LRU(addr, groupIdx);
    if(cacheLineSet[victim].valid){
        evictionFlag = 1;
    }
    // 更新缓存行和ru ins
    cacheLineSet[victim].valid = 1;
    cacheLineSet[victim].tag = addr.tag;
    cacheLineSet[victim].lru = nw_ins_idx;
    
    if(hitFlag)             hit++;
    if(missFlag)            miss++;
    if(evictionFlag)        eviction++;
}

int main(int argc, char ** argv)
{
    s = argv[2][0] - '0';
    E = argv[4][0] - '0';
    b = argv[6][0] - '0';
    setSize = (1 << s) * E;
    cacheLineSet = (CacheLine*)calloc(setSize, sizeof(CacheLine));
    FILE * fp;
    if((fp=fopen(argv[8],"r"))==NULL){ 
        printf("cannot open this file\n");
        exit(0);
    }
    char * line = (char*)malloc(25);
    while(fgets(line, 25, fp) != NULL){
        // printf("Ins: %s", line);
        OP = line[1];
        if(OP == ' '){
            continue;
        }
        int lineIdx = 3;
        while(line[++lineIdx] != ',');
        processAddr(&line[3], lineIdx-3);
        nw_ins_idx++;
    }
        
    printSummary(hit, miss, eviction);
    free(cacheLineSet);
    free(line);
    fclose(fp);
    return 0;
}

// test case:
// ./csim -s 1 -E 1 -b 1 -t traces/yi2.trace        
// ./csim -s 4 -E 2 -b 4 -t traces/yi.trace         
// ./csim -s 2 -E 1 -b 4 -t traces/dave.trace       
// ./csim -s 2 -E 1 -b 3 -t traces/trans.trace
// ./csim -s 2 -E 2 -b 3 -t traces/trans.trace
// ./csim -s 2 -E 4 -b 3 -t traces/trans.trace
// ./csim -s 5 -E 1 -b 5 -t traces/trans.trace
// ./csim -s 5 -E 1 -b 5 -t traces/long.trace