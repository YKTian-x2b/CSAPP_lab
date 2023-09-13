#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT * 2 - 1)) & ~0xf)

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12) //  Extend heap by this amount

#define MAX(x, y) ((x) > (y) ? (x) : (y))
// 把size和allocated位打包到一个字里
#define PACK(size, alloc) ((size) | (alloc))
// 读写地址p处的一个字
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
// 分别读取地址p处的size和allocated位
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PREV_ALLOC(p) (GET(p) & 0x2)
// 给定block ptr 求 footer header 地址
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
// 给定当前block ptr 求 上一个或下一个block
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-DSIZE))

// #define COMM_REALLOC
// #define COMM_FREE
// #define COMM_ALLOC
// #define COMM_INIT

#define ARRAYSIZE 10
void *heap_listp = NULL;
unsigned free_blks[ARRAYSIZE];
// INF-4097 4096-2049 2048-1025 1024-513 512-257 256-129 128-65 64-33 32-17 16-1

static int getArrIdx(int size)
{
    if (size > 4096)
        return 9;
    int base = 16;
    int i = 0;
    while ((base << i) < size)
    {
        i++;
    }
    return i;
}

static void insertNode(int arrIdx, unsigned node)
{
    unsigned tmp = GET(&free_blks[arrIdx]);
    PUT(&free_blks[arrIdx], node);
    PUT(node, tmp);
}

static void delNode(int arrIdx, unsigned node)
{
    // printf("delNode: arrIdx: %d, node: %p\n", arrIdx, node);
    char *nw = &free_blks[arrIdx];
    char *prev_free = NULL;
    while (nw)
    {
        if (GET(nw) == node)
        {
            prev_free = nw;
            break;
        }
        nw = (char *)GET(nw);
    }
    if(!prev_free){
        printf("delNode: no prev_free\n");
        exit(1);
    }
    unsigned int tmp = GET((unsigned*)node);
    PUT(prev_free, tmp);
}

static void *coalesce(char *bp)
{
    char *res = bp;
    char *next_bp = NEXT_BLKP(bp);
    int bp_size = GET_SIZE(HDRP(bp));
    int next_alloc = GET_ALLOC(HDRP(next_bp));
    int prev_alloc = GET_PREV_ALLOC(HDRP(bp));

    if (prev_alloc && next_alloc)
    {
        // printf("prev_alloc && next_alloc\n");
        insertNode(getArrIdx(bp_size), bp);
    }
    else if (prev_alloc && !next_alloc)
    {
        // printf("prev_alloc && !next_alloc\n");
        int next_bp_size = GET_SIZE(HDRP(next_bp));
        delNode(getArrIdx(next_bp_size) , next_bp);
        insertNode(getArrIdx(bp_size + next_bp_size), bp);
    }
    else if (!prev_alloc && next_alloc)
    {
        // printf("!prev_alloc && next_alloc\n");
        char *prev_bp = PREV_BLKP(bp);
        int prev_bp_size = GET_SIZE(HDRP(prev_bp));
        delNode(getArrIdx(prev_bp_size) , prev_bp);
        insertNode(getArrIdx(bp_size + prev_bp_size), prev_bp);
    }
    else
    {
        // printf("!prev_alloc && !next_alloc\n");
        char *prev_bp = PREV_BLKP(bp);
        int prev_bp_size = GET_SIZE(HDRP(prev_bp));
        int next_bp_size = GET_SIZE(HDRP(next_bp));
        delNode(getArrIdx(prev_bp_size) , prev_bp);
        delNode(getArrIdx(next_bp_size) , next_bp);
        insertNode(getArrIdx(bp_size + prev_bp_size + next_bp_size), prev_bp);
    }

    if (!next_alloc)
    {
        unsigned bothSize = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(next_bp));
        PUT(HDRP(bp), PACK(bothSize, prev_alloc));
        PUT(FTRP(bp), PACK(bothSize, prev_alloc));
    }
    if (!prev_alloc)
    {
        char *prev_bp = PREV_BLKP(bp);
        unsigned bothSize = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(prev_bp));
        PUT(FTRP(bp), PACK(bothSize, 2));
        PUT(HDRP(prev_bp), PACK(bothSize, 2));
        res = prev_bp;
    }

    return res;
}

static void *extend_heap(unsigned words)
{
    char *bp;
    unsigned size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
    {
        return NULL;
    }

    char *last_block = heap_listp;
    char *nw = heap_listp;
    while (GET_SIZE(HDRP(nw)) != 0)
    {
        last_block = nw;
        nw = NEXT_BLKP(nw);
    }

    if (GET_ALLOC(HDRP(last_block)))
    {
        PUT(HDRP(bp), PACK(size, 2));
        PUT(FTRP(bp), PACK(size, 2));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    }
    else
    {
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    }
    // 如果扩展分配前heap的最后一个块是空闲的，就合并新空闲空间和该块
    return coalesce(bp);
}

static void *find_fit(unsigned asize)
{
    // printf("find_fit()\n");
    int arrIdxTmp = getArrIdx(asize);
    char *nw;
    while (arrIdxTmp <= 9)
    {
        nw = (char *)free_blks[arrIdxTmp];
        while (nw)
        {
            if (GET_SIZE(HDRP(nw)) >= asize)
            {
                return nw;
            }
            nw = (char *)GET(nw);
        }
        arrIdxTmp++;
    }
    return NULL;
}

static void *best_fit(unsigned asize)
{
    // printf("best_fit()\n");
    int thisArrIdx = 0;
    unsigned minFitSize = -1;
    char * minFitPtr = NULL;

    int arrIdxTmp = getArrIdx(asize);
    char *nw;
    while (arrIdxTmp <= 9)
    {
        nw = (char *)free_blks[arrIdxTmp];
        while (nw)
        {
            unsigned nwSize = GET_SIZE(HDRP(nw));
            if (nwSize >= asize)
            {
                thisArrIdx = 1;
                if(nwSize < minFitSize){
                    minFitSize = nwSize;
                    minFitPtr = nw;
                }
            }
            nw = (char *)GET(nw);
        }
        if(thisArrIdx){
            break;
        }
        arrIdxTmp++;
    }
    return minFitPtr;
}

static void place(char *bp, unsigned asize)
{
    // printf("place()\n");
    unsigned allSize = GET_SIZE(HDRP(bp));
    unsigned remSize = allSize - asize;
    unsigned prev_alloc = GET_PREV_ALLOC(HDRP(bp));

    if (remSize)
    {
        int srcArrIdx = getArrIdx(allSize);
        delNode(srcArrIdx, bp);
        //
        PUT(HDRP(bp), PACK(asize, (1 | prev_alloc)));
        //
        char *next_bp = NEXT_BLKP(bp);
        int desArrIdx = getArrIdx(remSize);
        insertNode(desArrIdx, next_bp);
        //
        PUT(HDRP(next_bp), PACK(remSize, 2));
        PUT(FTRP(next_bp), PACK(remSize, 2));
    }
    else
    {
        //
        PUT(HDRP(bp), PACK(asize, (1 | prev_alloc)));
        //
        int srcArrIdx = getArrIdx(allSize);
        delNode(srcArrIdx, bp);
        //
        char *next_bp = NEXT_BLKP(bp);
        unsigned next_bp_size = GET_SIZE(HDRP(next_bp));
        unsigned next_bp_alloc = GET_ALLOC(HDRP(next_bp));
        PUT(HDRP(next_bp), PACK(next_bp_size, (2 | next_bp_alloc)));
    }
}

/*
 * mm_init - initialize the malloc package.
 */
// 初始化 安排二字对齐块 序言块和结尾块 并扩展一个CHUNK
int mm_init(void)
{
    // printf("\n\n=================\n");
    mem_init();
    memset(free_blks, 0, sizeof(unsigned) * ARRAYSIZE);
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
    {
        return -1;
    }
    PUT(heap_listp, 0);
    PUT(heap_listp + WSIZE, PACK(DSIZE, 1));
    // 序言的脚部改为显示空闲链表的头部
    PUT(heap_listp + 2 * WSIZE, 0);
    PUT(heap_listp + 3 * WSIZE, PACK(0, 1));
    heap_listp += 2 * WSIZE;

    if (extend_heap((1 << 6) / WSIZE) == NULL)
    {
        return -1;
    }

#ifdef COMM_INIT
    printf("mm_init\n");
    mm_check();
#endif
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
#ifdef COMM_ALLOC
    printf("mm_malloc: begin\n");
    mm_check();
#endif
    unsigned asize;
    unsigned extendSize;
    char *bp;
    if (size == 0)
        return NULL;
    // 一字是header
    asize = ALIGN(size + WSIZE);
    // printf("--ALIGN(size+DSIZE): %u\n", asize);
    // if ((bp = find_fit(asize)) != NULL)
    if ((bp = best_fit(asize)) != NULL)
    {
        place(bp, asize);
#ifdef COMM_ALLOC
        printf("mm_malloc: find fit\n");
        mm_check();
#endif
        return bp;
    }

    extendSize = MAX(asize, CHUNKSIZE);
    // printf("extendSize: %d\n", extendSize);
    if ((bp = extend_heap(extendSize / WSIZE)) == NULL)
        return NULL;

    place(bp, asize);
#ifdef COMM_ALLOC
    printf("mm_malloc: extend_heap\n");
    mm_check();
#endif
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
#ifdef COMM_FREE
    printf("mm_free begin\n");
    mm_check();
#endif
    unsigned size = GET_SIZE(HDRP(ptr));
    unsigned prev_alloc = GET_PREV_ALLOC(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, prev_alloc));
    PUT(FTRP(ptr), PACK(size, prev_alloc));

    char *next_blk_hdrp = HDRP(NEXT_BLKP(ptr));
    unsigned next_blk_size = GET_SIZE(next_blk_hdrp);
    unsigned next_blk_alloc = GET_ALLOC(next_blk_hdrp);
    PUT(next_blk_hdrp, PACK(next_blk_size, (next_blk_alloc)));
    coalesce(ptr);
#ifdef COMM_FREE
    printf("mm_free\n");
    mm_check();
#endif
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    char * next_blk = NEXT_BLKP(ptr);
    char * next_blk_hdrp = HDRP(next_blk);
    if(!GET_ALLOC(next_blk_hdrp)){
        unsigned allSize = GET_SIZE(next_blk_hdrp) + GET_SIZE(HDRP(ptr));
        unsigned asize = ALIGN(size + WSIZE);
        if(allSize >= asize){
            unsigned remSize = allSize - asize;
            unsigned prev_alloc = GET_PREV_ALLOC(HDRP(ptr));
            if (remSize)
            {
                int srcArrIdx = getArrIdx(GET_SIZE(next_blk_hdrp));
                delNode(srcArrIdx, next_blk);
                //
                PUT(HDRP(ptr), PACK(asize, (1 | prev_alloc)));
                char *next_bp = NEXT_BLKP(ptr);
                int desArrIdx = getArrIdx(remSize);
                insertNode(desArrIdx, next_bp);
                //
                PUT(HDRP(next_bp), PACK(remSize, 2));
                PUT(FTRP(next_bp), PACK(remSize, 2));
            }
            else
            {
                //
                int srcArrIdx = getArrIdx(GET_SIZE(next_blk_hdrp));
                delNode(srcArrIdx, next_blk);
                //
                PUT(HDRP(ptr), PACK(asize, (1 | prev_alloc)));
                char *next_bp = NEXT_BLKP(ptr);
                unsigned next_bp_size = GET_SIZE(HDRP(next_bp));
                unsigned next_bp_alloc = GET_ALLOC(HDRP(next_bp));
                PUT(HDRP(next_bp), PACK(next_bp_size, (2 | next_bp_alloc)));
                if(!next_bp_alloc){
                    PUT(FTRP(next_bp), PACK(next_bp_size, (2 | next_bp_alloc)));
                }
                
            }
#ifdef COMM_REALLOC
            printf("mm_realloc: special\n");
            mm_check();
#endif
            return ptr;
        }
    }

    void *oldptr = ptr;
    void *newptr;
    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    if (!oldptr)
    {
        return newptr;
    }
    unsigned copySize = GET_SIZE(HDRP(ptr)) - WSIZE;
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
#ifdef COMM_REALLOC
    printf("mm_realloc: common\n");
    mm_check();
#endif
    return newptr;
}

int mm_check(void)
{
    printf("== mm_check begin == \n");

    char *bp = heap_listp;
    while (GET_SIZE(HDRP(bp)) != 0)
    {
        bp = NEXT_BLKP(bp);
    }
    printf("[mem_start_brk : mem_brk]=[%p : %p], mem_heapsize: %u\n", mem_heap_lo(), mem_heap_hi(), mem_heapsize());
    printf("[mem_heap_lo : mem_heap_hi]=[%p : %p], mem_heapsize: %u\n",
           heap_listp, bp, bp-((char*)heap_listp)-DSIZE);

    printf("-- heap_listp_check --\n");
    bp = heap_listp;
    int nwSize;
    int totalSize = 0;
    while ((nwSize = GET_SIZE(HDRP(bp))) != 0)
    {
        char* hdrp = HDRP(bp);
        if(GET_ALLOC(hdrp)){
            totalSize += GET_SIZE(hdrp);
        }
        printf("block_lo : %p, block_size: %u, block_alloc: %u, prev_block_alloc: %u\n",
               bp, GET_SIZE(hdrp), GET_ALLOC(hdrp), GET_PREV_ALLOC(hdrp));
        bp = NEXT_BLKP(bp);
    }
    
    printf("-- free_blk_check --\n");
    int arrIdxTmp = 0;
    char *nw;
    while (arrIdxTmp <= 9)
    {
        printf("arrIdx%d [", arrIdxTmp);
        nw = (char *)free_blks[arrIdxTmp];
        while (nw)
        {
            printf("lo: %p, sz: %u, alo: %u, pre_alo: %u;   ",
                   nw, GET_SIZE(HDRP(nw)), GET_ALLOC(HDRP(nw)), GET_PREV_ALLOC(HDRP(nw)));
            nw = (char *)GET(nw);
        }
        arrIdxTmp++;
        printf("]\n", nw);
    }

    printf("== mm_check end == \n");
    return 0;
}
