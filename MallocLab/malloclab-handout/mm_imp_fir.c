/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
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
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
// size_t的size
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

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
// 给定block ptr 求 footer header 地址
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
// 给定当前block ptr 求 上一个或下一个block
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-DSIZE))

static void *head_listp = NULL;

static void *coalesce(char *bp)
{
    char *res = bp;
    char *prev_bp = PREV_BLKP(bp);
    char *next_bp = NEXT_BLKP(bp);
    if (!GET_ALLOC(HDRP(next_bp)))
    {
        unsigned bothSize = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(next_bp));
        PUT(HDRP(bp), PACK(bothSize, 0));
        PUT(FTRP(bp), PACK(bothSize, 0));
    }
    if (!GET_ALLOC(FTRP(prev_bp)))
    {
        unsigned bothSize = GET_SIZE(HDRP(bp)) + GET_SIZE(FTRP(prev_bp));
        PUT(FTRP(bp), PACK(bothSize, 0));
        PUT(HDRP(prev_bp), PACK(bothSize, 0));
        res = prev_bp;
    }
    return res;
}

static void *find_fit(unsigned asize)
{
    char *bp = NEXT_BLKP(head_listp);
    unsigned nowSize;
    // printf("blockSizes: [");
    while ((nowSize = GET_SIZE(HDRP(bp))) != 0)
    {
        // printf("%u, ", nowSize);
        if (GET_ALLOC(HDRP(bp)) == 0 && nowSize >= asize)
        {
            // printf("]\n");
            return bp;
        }
        bp = NEXT_BLKP(bp);
    }
    // printf("end of head\n");
    return NULL;
}

static void place(char *bp, unsigned asize)
{
    unsigned allSize = GET_SIZE(HDRP(bp));
    unsigned remSize = allSize - asize;
    if (remSize){
        PUT(FTRP(bp), PACK(remSize, 0));
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(remSize, 0));
    }
    else{
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
    } 
}

static void *extend_heap(unsigned words)
{
    char *bp;
    unsigned size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
    {
        return NULL;
    }
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    // 如果扩展分配前heap的最后一个块是空闲的，就合并新空闲空间和该块
    return coalesce(bp);
}
/*
 * mm_init - initialize the malloc package.
 */
// 初始化 安排二字对齐块 序言块和结尾块 并扩展一个CHUNK
int mm_init(void)
{
    // printf("\n=================\n");
    mem_init();
    if ((head_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
    {
        return -1;
    }
    PUT(head_listp, 0);
    PUT(head_listp + WSIZE, PACK(DSIZE, 1));
    PUT(head_listp + 2 * WSIZE, PACK(DSIZE, 1));
    PUT(head_listp + 3 * WSIZE, PACK(0, 1));
    head_listp += 2 * WSIZE;

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
    {
        return -1;
    }
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // int newsize = ALIGN(size + SIZE_T_SIZE);
    // void *p = mem_sbrk(newsize);
    // if (p == (void *)-1)
    // return NULL;
    // else {
    //     *(unsigned *)p = size;
    //     return (void *)((char *)p + SIZE_T_SIZE);
    // }
    unsigned asize;
    unsigned extendSize;
    char *bp;
    if (size == 0)
        return NULL;
    // 两字是footer和header 另外两字是满足size需求的最小对齐字数
    asize = ALIGN(size + DSIZE);
    // printf("--ALIGN(size+DSIZE): %u\n", asize);
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }
    // printf("extendSize");
    extendSize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendSize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    unsigned size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    if(!oldptr){
        return newptr;
    }
    unsigned copySize = GET_SIZE(HDRP(ptr)) - DSIZE;
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

int mm_check(void)
{
    printf("[mem_heap_lo : mem_heap_hi]=[%p : %p], mem_heapsize: %u\n",
           mem_heap_lo, mem_heap_hi, mem_heapsize);
    char *bp = head_listp;
    int nwSize;
    while ((nwSize = GET_SIZE(HDRP(bp))) != 0)
    {
        printf("[block_lo : block_hi]=[%p : %p], mem_heapsize: %u\n",
               HDRP(bp), FTRP(bp), GET_SIZE(HDRP(bp)));
        bp = NEXT_BLKP(bp);
    }
    return 0;
}
