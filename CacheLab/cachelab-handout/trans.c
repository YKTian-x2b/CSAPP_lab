/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

void transpose_32x32(int M, int N, int A[N][M], int B[M][N]);
void transpose_64x64(int M, int N, int A[N][M], int B[M][N]);
void transpose_61x67(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
// 数组A和B的MemAddr导致A的对角线元素所在line和B的对角线元素所在line会被直接映射到相同缓存行
// 因此对角线元素的转置读后写会导致缓存行抖动 为了避免这个问题，列读行写会忽略对角线元素
// 对角线元素A(k+1, k+1)的读会在A(k+1, k)元素的读取操作后完成，同时写入B(k+1, k+1)
// 这会导致对应缓存行成为B的line，这和下次迭代造成的影响一样，所以不会影响cache的时间局部性
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if(N == 32){
        transpose_32x32(M, N, A, B);
    }
    else if(N == 64){
        transpose_64x64(M, N, A, B);
    }
    else{
        transpose_61x67(M, N, A, B);
    }
}

// 32*32的解决方案
// 在8*8的情况下：
// 数组A和B的MemAddr导致A的对角线元素所在line和B的对角线元素所在line会被直接映射到相同缓存行
// 因此对角线元素的转置读后写会导致缓存行抖动 为了避免这个问题：
// 1. 列读行写会忽略对角线元素
// 2. 对角线元素A(k+1, k+1)的读会在A(k+1, k)元素的读取操作后完成，同时写入B(k+1, k+1)
//     - 这会导致对应缓存行成为B的line，这和下次迭代造成的影响一样，所以不会影响cache的时间局部性
char transpose_32x32_desc[] = "Transpose 32x32";
void transpose_32x32(int M, int N, int A[N][M], int B[M][N])
{
    int stride = 8;     // 8*8的正方形block
    for(int i = 0; i < N; i+=stride){
        for(int j = 0; j < M; j+=stride){
            B[j][i] = A[i][j];                      // 对角线的第一个元素无法处理，特殊处理，并将B[j]放入缓存行
            for(int k = 0; k < stride; k++){
                for(int l = 0; l < stride; l++){
                    if(k == l){
                        continue;                   // 避免对角线元素的读后写
                    }
                    B[j+k][i+l] = A[i+l][j+k];
                    if(k + 1 == l)                  // A(k+1, k)元素的读取操作后完成对角线元素A(k+1, k+1)的读后写
                        B[j+k+1][i+l] = A[i+l][j+k+1];
                }
            }
        }
    }
}

// 64*64的解决方案
// 在8*8的情况下：
// 数组A/数组B的MemAddr每隔4*64个元素就会被直接映射到相同缓存行
// 为了解决该问题，8*8block被拆分成4角的4*4block，并按照四个象限以2143或2341顺序处理的方式来尽量满足cache的时间局部性
// 仍然是对角线元素的转置读后写会导致缓存行抖动 为了避免这个问题，对角线元素所在的4*4block列读行写采取和32*32问题相同的解决方案
// 非 对角线元素所在8*8块 不必考虑该问题
// 为了尽量减少miss, 把部分行写入寄存器 避免部分 miss eviction
char transpose_64x64_desc[] = "Transpose 64x64";
void transpose_64x64(int M, int N, int A[N][M], int B[M][N])
{
    int half_stride = 4;
    int stride = 8;
    // 对角线元素所在8*8块的处理较为复杂，放在else里
    // 非 对角线元素所在8*8块 的处理较简单，放在if里
    for(int i = 0; i < N; i+=stride){
        for(int j = 0; j < M; j+=stride){
            // 为了尽量减少miss 我们用寄存器存储一个4*4 A block的两行元素，这将避免迭代最后的两个eviction
            if(i != j){
                // 读A 第2象限
                for(int k = 0; k < half_stride; k++){
                    for(int l = 0; l < half_stride; l++){
                        B[j+k][i+l] = A[i+l][j+k];
                    }
                }
                // 提前寄存器缓存
                int val0 = A[i][j+half_stride];
                int val1 = A[i][j+half_stride+1];
                int val2 = A[i][j+half_stride+2];
                int val3 = A[i][j+half_stride+3];

                int val4 = A[i+1][j+half_stride];
                int val5 = A[i+1][j+half_stride+1];
                int val6 = A[i+1][j+half_stride+2];
                int val7 = A[i+1][j+half_stride+3];
                // A 3
                for(int k = 0; k < half_stride; k++){
                    for(int l = half_stride; l < stride; l++){
                        B[j+k][i+l] = A[i+l][j+k];
                    }
                }
                // A 4
                for(int k = half_stride; k < stride; k++){
                    for(int l = half_stride; l < stride; l++){
                        B[j+k][i+l] = A[i+l][j+k];
                    }
                }
                // A 1
                // 提前寄存器缓存了
                // B[4][0] B[5][0] ... B[7][0]
                // B[4][1] B[5][1] ... B[7][1]
                for(int k = half_stride; k < stride; k++){
                    for(int l = 2; l < half_stride; l++){
                        B[j+k][i+l] = A[i+l][j+k];
                    }
                }
                B[j+half_stride][i] = val0;
                B[j+half_stride+1][i] = val1;
                B[j+half_stride+2][i] = val2;
                B[j+half_stride+3][i] = val3;

                B[j+half_stride][i+1] = val4;
                B[j+half_stride+1][i+1] = val5;
                B[j+half_stride+2][i+1] = val6;
                B[j+half_stride+3][i+1] = val7;
            }
            else{
                // 这些局部变量顺着cacheline的局部性调整位置 避免miss
                int val1;                                                       // 存A[3,7]
                // A0 A1 指 A 8*8矩阵的上半个4*8
                // B0 B1 指 B 8*8矩阵的上半个4*8
                // A左上的列读会导致 缓存行形成 A0 A0 A0 B0
                for(int k = 0; k < half_stride; k++){
                    if(k == 0){
                        val1 = A[i+half_stride-1][j+stride-1];
                    }
                    for(int l = 0; l < half_stride; l++){
                        if(k == l){
                            continue;
                        }
                        B[j+k][i+l] = A[i+l][j+k];
                        if(k + 1 == l){
                            B[j+k+1][i+l] = A[i+l][j+k+1];
                        } 
                    }
                }
                int val0 = A[i][j];                                              // 存A[0,0]
                B[j+stride-1][i+half_stride-1] = val1;
                // 为了 A0 A0 A0 的时间局部性 第二个循环处理 A右上(B左下) 且按索引高到低写
                // 缓存行形成 B1 A0 A0 A0 
                for(int k = stride-1; k >= half_stride; k--){
                    for(int l = 0; l < half_stride; l++){
                        if(k == l+half_stride){
                            continue;
                        }
                        B[j+k][i+l] = A[i+l][j+k];
                        if(k-1 == l+half_stride){
                            B[j+k-1][i+l] = A[i+l][j+k-1];
                        }
                    }
                }   
                int val3 = A[i+stride-1][j+half_stride-1];                      // 存 A[7,3]
                // 为了B1 的时间局部性 第三个循环处理 A右下(B右下) 且按索引低到高写
                // 缓存行形成 A1 A1 A1 B1
                for(int k = half_stride; k < stride; k++){
                    for(int l = half_stride; l < stride; l++){
                        if(k == l){
                            continue;
                        }
                        B[j+k][i+l] = A[i+l][j+k];
                        if(k+1 == l){
                            B[j+k+1][i+l] = A[i+l][j+k+1];
                        }
                    }
                }
                int val2 = A[i+half_stride][j+half_stride];                     // 存 A[4,4]
                // 为了 A1 A1 A1 的时间局部性 第四个循环处理 A左下(B右上) 且按索引高到低写
                // 缓存行形成 B0 A1 A1 A1
                B[j+half_stride-1][i+stride-1] = val3;
                for(int k = half_stride-1; k >= 0; k--){
                    for(int l = half_stride; l < stride; l++){
                        if(l == k+half_stride){
                            continue;
                        }
                        B[j+k][i+l] = A[i+l][j+k];
                        if(k-1+half_stride == l){
                            B[j+k-1][i+l] = A[i+l][j+k-1];
                        }
                    }
                }
                B[j][i] = val0;
                // 这个没办法 只能miss一下
                B[j+half_stride][i+half_stride] = val2;
            }
        }
    }
}

// 61*67的解决方案
// 在6*6的情况下:
// 为了尽量减少miss, 把部分行/列写入寄存器 利用时间局部性 避免部分 miss eviction
char transpose_61x67_desc[] = "Transpose 61x67";
void transpose_61x67(int M, int N, int A[N][M], int B[M][N]){
    // int stride = 6;
    // int NN = (N - N % 6);
    // int MM = (M - M % 6); 
    int v0, v1, v2, v3, v4, v5;             // 顺便把A的最后一列读一下
    int v6, v7, v8, v9, v10, v11;           // 顺便把B的最后一列写一下
    // 以6*6block为一组
    for(int i = 0; i < (N - N % 6); i+=6){
        for(int j = 0; j < (M - M % 6); j+=6){
            // 当A是最后一个行block时,顺便读一下A的最后一行,以供B最后一列写
            if(i+6 == (N - N % 6)){
                v6 = A[(N - N % 6)][j+0];
                v7 = A[(N - N % 6)][j+1];
                v8 = A[(N - N % 6)][j+2];
                v9 = A[(N - N % 6)][j+3];
                v10 = A[(N - N % 6)][j+4];
                v11 = A[(N - N % 6)][j+5];
            }
            // 核心代码还是最简易的 B[j+k][i+l] = A[i+l][j+k];
            for(int k = 0; k < 6; k++){
                for(int l = 0; l < 6; l++){
                    B[j+k][i+l] = A[i+l][j+k];
                    // 当A是最后一个列block时,顺便读一下A的最后一列,以供B最后一行写
                    if(j+6 >= (M - M % 6) && k == 6-1){
                        if(l == 0)
                            v0 = A[i+l][M-1];
                        if(l == 1)
                            v1 = A[i+l][M-1];
                        if(l == 2)
                            v2 = A[i+l][M-1];
                        if(l == 3)
                            v3 = A[i+l][M-1];
                        if(l == 4)
                            v4 = A[i+l][M-1];
                        if(l == 5)
                            v5 = A[i+l][M-1];
                    }
                }
                // 当A是最后一个行block时,B是最后一个列block,顺便写一下B的最后一列
                if(i+6 == (N - N % 6)){
                    if(k == 0)
                        B[j+k][(N - N % 6)] = v6;
                    if(k == 1)
                        B[j+k][(N - N % 6)] = v7;
                    if(k == 2)
                        B[j+k][(N - N % 6)] = v8;
                    if(k == 3)
                        B[j+k][(N - N % 6)] = v9;
                    if(k == 4)
                        B[j+k][(N - N % 6)] = v10;
                    if(k == 5)
                        B[j+k][(N - N % 6)] = v11;
                }
            }
            // 当A是最后一个列block时,B是最后一个行block,就把最后一行也写了
            if(j+6 >= (M - M % 6)){
                B[M-1][i] = v0;
                B[M-1][i+1] = v1;
                B[M-1][i+2] = v2;
                B[M-1][i+3] = v3;
                B[M-1][i+4] = v4;
                B[M-1][i+5] = v5;
            }
        }
    }
    // 处理一下右下角的1*1
    B[M-1][N-1] = A[N-1][M-1];
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}