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
// #define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define ALIGN(size) (((size) + (ALIGNMENT*2 - 1)) & ~0xf)
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
#define GET_PREV_ALLOC(p) (GET(p) & 0x2)
// 给定block ptr 求 footer header 地址
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
// 给定当前block ptr 求 上一个或下一个block
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-DSIZE))

// #define COMM

static void *heap_listp = NULL;
static void *free_blk_listp = NULL;

static void *coalesce(char *bp)
{
    char *res = bp;
    char *next_bp = NEXT_BLKP(bp);
    int next_alloc = GET_ALLOC(HDRP(next_bp));
    int prev_alloc = GET_PREV_ALLOC(HDRP(bp));

    if(prev_alloc && next_alloc){
        // printf("prev_alloc && next_alloc\n");
        char * prev_free = free_blk_listp;
        char * nw = GET(free_blk_listp);
        while(nw){
           if(nw < bp){
                prev_free = nw;
            }
            else{
                break;
            }
            nw = (char*)GET(nw);
        }
        unsigned int tmp = GET(prev_free);
        // 在prev_free指向的空间里放入地址bp 表示prev_free指向的block的下一个freeBlock的地址是bp
        PUT(prev_free, bp);
        // 在bp指向的空间里放入地址 *prev_free 表示bp指向的block的下一个freeBlock的地址是*prev_free
        PUT(bp, tmp);
    }
    else if(prev_alloc && !next_alloc){
        // printf("prev_alloc && !next_alloc\n");
        char * prev_free = free_blk_listp;
        char * nw = GET(free_blk_listp);
        while(nw){
            if(nw < bp){
                prev_free = nw;
            }
            else{
                break;
            }
            nw = (char*)GET(nw);
        }
        unsigned int tmp = GET(GET(prev_free));
        PUT(prev_free, bp);
        PUT(bp, tmp);
    }
    else if(!prev_alloc && next_alloc){
        // printf("!prev_alloc && next_alloc\n");
        char *prev_bp = PREV_BLKP(bp);
        char * prev_free = free_blk_listp;
        char * nw = GET(free_blk_listp);
        while(nw){
            if(nw < prev_bp){
                prev_free = nw;
            }
            else{
                break;
            }
            nw = (char*)GET(nw);
        }
        // printf("bp: %p\n", bp);
        // printf("prev_bp: %p\n", prev_bp);
        // printf("prev_free: %p\n", prev_free);
        unsigned int tmp = GET(GET(prev_free));
        
        PUT(prev_free, prev_bp);
        PUT(prev_bp, tmp);
        // mm_check();
    }
    else{
        // printf("!prev_alloc && !next_alloc\n");
        char *prev_bp = PREV_BLKP(bp);
        char * prev_free = free_blk_listp;
        char * nw = GET(free_blk_listp);
        while(nw){
            if(nw < prev_bp){
                prev_free = nw;
            }
            else{
                break;
            }
            nw = (char*)GET(nw);
        }
        unsigned int tmp = GET(GET(GET(prev_free)));
        PUT(prev_free, prev_bp);
        PUT(prev_bp, tmp);
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

    char * last_free_blk = NULL;
    char * nw = GET(free_blk_listp);
    while(nw){
        last_free_blk = nw;
        nw = (char*)GET(nw);
    }

    // printf("extend_heap()1\n");

    if(last_free_blk == NULL || NEXT_BLKP(last_free_blk) != bp){
        // printf("extend_heap() if \n");
        PUT(HDRP(bp), PACK(size, 2));
        PUT(FTRP(bp), PACK(size, 2));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    }
    else{
        // printf("extend_heap() else \n");
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    }
    
    // printf("extend_heap() end -> coalesce()\n");
    // 如果扩展分配前heap的最后一个块是空闲的，就合并新空闲空间和该块
    return coalesce(bp);
}

static void *find_fit(unsigned asize)
{
    // 
    char * nw = GET(free_blk_listp);
    while(nw){
        if(GET_SIZE(HDRP(nw)) >= asize){
            return nw;
        }
        nw = (char*)GET(nw);
    }
    return NULL;

}

static void place(char *bp, unsigned asize)
{
    unsigned allSize = GET_SIZE(HDRP(bp));
    unsigned remSize = allSize - asize;
    unsigned prev_alloc = GET_PREV_ALLOC(HDRP(bp));

    if (remSize){
        PUT(HDRP(bp), PACK(asize, (1|prev_alloc)));

        char * next_bp = NEXT_BLKP(bp);

        PUT(HDRP(next_bp), PACK(remSize, 2));
        PUT(FTRP(next_bp), PACK(remSize, 2));
        // 
        char * prev_free = free_blk_listp;
        while(GET(prev_free) != (unsigned *)bp){
            prev_free = (char*)GET(prev_free);
        }
        unsigned int tmp = GET(GET(prev_free));
        PUT(prev_free, next_bp);
        PUT(next_bp, tmp);
    }
    else{
        PUT(HDRP(bp), PACK(asize, (1|prev_alloc)));
        //
        char * prev_free = free_blk_listp;
        while(GET(prev_free) != (unsigned *)bp){
            prev_free = (char*)GET(prev_free);
        }
        unsigned int tmp = GET(GET(prev_free));
        PUT(prev_free, tmp);

        char * next_bp = NEXT_BLKP(bp);
        unsigned next_bp_size = GET_SIZE(HDRP(next_bp));
        unsigned next_bp_alloc = GET_ALLOC(HDRP(next_bp));
        PUT(HDRP(next_bp), PACK(next_bp_size, (2|next_bp_alloc)));
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
    free_blk_listp = heap_listp;

    // printf("mem_init() before extend_heap\n");
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
    {
        return -1;
    }
    // printf("mem_init() after extend_heap\n");
    // mm_check();
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    unsigned asize;
    unsigned extendSize;
    char *bp;
    if (size == 0)
        return NULL;
    // 一字是header
    asize = ALIGN(size + WSIZE);
    // printf("--ALIGN(size+DSIZE): %u\n", asize);
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
#ifdef COMM
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
#ifdef COMM
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
    unsigned size = GET_SIZE(HDRP(ptr));
    unsigned prev_alloc = GET_PREV_ALLOC(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, prev_alloc));
    PUT(FTRP(ptr), PACK(size, prev_alloc));

    char * next_blk_hdrp = HDRP(NEXT_BLKP(ptr));
    unsigned next_blk_size = GET_SIZE(next_blk_hdrp);
    unsigned next_blk_alloc = GET_ALLOC(next_blk_hdrp);
    PUT(next_blk_hdrp, PACK(next_blk_size, (next_blk_alloc)));
    coalesce(ptr);
#ifdef COMM
    printf("mm_free\n");
    mm_check();
#endif
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    char * nw = GET(free_blk_listp);
    while(nw && nw <= ptr){
        nw = (char*)GET(nw);
    }
    if(nw && nw > ptr){
        char * next_blk = NEXT_BLKP(ptr);
        unsigned allSize = GET_SIZE(next_blk) + GET_SIZE(ptr);
        if(nw == next_blk &&  allSize >= size){
            unsigned remSize = allSize - size;
            unsigned prev_alloc = GET_PREV_ALLOC(HDRP(ptr));

            if (remSize){
                PUT(HDRP(ptr), PACK(size, (1|prev_alloc)));

                char * next_bp = NEXT_BLKP(ptr);

                PUT(HDRP(next_bp), PACK(remSize, 2));
                PUT(FTRP(next_bp), PACK(remSize, 2));
                // 
                char * prev_free = free_blk_listp;
                while(GET(prev_free) != (unsigned *)ptr){
                    prev_free = (char*)GET(prev_free);
                }
                unsigned int tmp = GET(GET(prev_free));
                PUT(prev_free, next_bp);
                PUT(next_bp, tmp);
            }
            else{
                PUT(HDRP(ptr), PACK(size, (1|prev_alloc)));
                //
                char * prev_free = free_blk_listp;
                while(GET(prev_free) != (unsigned *)ptr){
                    prev_free = (char*)GET(prev_free);
                }
                unsigned int tmp = GET(GET(prev_free));
                PUT(prev_free, tmp);

                char * next_bp = NEXT_BLKP(ptr);
                unsigned next_bp_size = GET_SIZE(HDRP(next_bp));
                unsigned next_bp_alloc = GET_ALLOC(HDRP(next_bp));
                PUT(HDRP(next_bp), PACK(next_bp_size, (2|next_bp_alloc)));
            } 
#ifdef COMM
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
    if(!oldptr){
        return newptr;
    }
    unsigned copySize = GET_SIZE(HDRP(ptr)) - WSIZE;
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
#ifdef COMM
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
    printf("[mem_heap_lo : mem_heap_hi]=[%p : %p], mem_heapsize: %u\n",
           heap_listp, bp, bp-((char*)heap_listp)-DSIZE);
    
    printf("-- heap_listp_check --\n");
    bp = heap_listp;
    int nwSize;
    while ((nwSize = GET_SIZE(HDRP(bp))) != 0)
    {
        char* hdrp = HDRP(bp);
        printf("block_lo : %p, block_size: %u, block_alloc: %u, prev_block_alloc: %u\n",
               hdrp, GET_SIZE(hdrp), GET_ALLOC(hdrp), GET_PREV_ALLOC(hdrp));
        bp = NEXT_BLKP(bp);
    }

    printf("-- free_blk_check --\n");
    char * nw = GET(free_blk_listp);
    while(nw){
        char* hdrp = HDRP(nw);
        printf("block_lo : %p, block_size: %u, block_alloc: %u, prev_block_alloc: %u\n",
               hdrp, GET_SIZE(hdrp), GET_ALLOC(hdrp), GET_PREV_ALLOC(hdrp));
        nw = (char*)GET(nw);
    }
    printf("== mm_check end == \n");
    return 0;
}
