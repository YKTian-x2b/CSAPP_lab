#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// arr[i]<=6 && arr[i]互不相等
int arr[6];
// 这里M就是从rsp+32开始的longlong数组 6个元素
int M[];
int rsp = 65536;

int read_six_numbers(int *);
// 答案是4 3 2 1 6 5
void phase_6(char * input){
    read_six_numbers(arr);
    // 523--542
    int r12d = 0;
    for(int i = 0; ; i++){
        int var1 = arr[i] - 1;
        if(var1 > 5){
            printf("Bomb!!!");
            return;
        }
        
        r12d++;
        if(r12d==6){
            break;
        }
        // 1-5 2-5 ... 5-5
        for(int var2 = r12d; var2 <=5; var2++){
            int var3 = arr[var2];
            if(arr[i] == var3){
                printf("Bomb!!!");
                return;
            }
        }
    }
    // 543--551
    // arr[i] = 7-arr[i];
    // int var4 = 7;
    for(int i = 0; i != 6; i++){
        // int var5 = var4;
        int var5 = 7;
        var5 -= arr[i];
        arr[i] = var5;      
    }

    // 552-569
    // idx=[0,1,2,3,4,5]
    // arr=[x0,x1,x2,...x5]
    // M[rsp+32+idx*8] = 0x6032d0+16*(7-idx);
    for(int idx = 0; idx != 6; idx++){
        if(arr[idx] <= 1){
            M[idx*8+32+rsp] = 0x6032d0;
        }
        else{
            //
            int var7 = 0x6032d0;
            for(int var6 = 1; var6 != arr[idx]; var6++){
                var7 = M[var7+8];
            }
            M[idx*8+32+rsp] = var7;
        }
    }
    // 570-580
    // M[M[32]+8]=M[40]
    // M[M[40]+8]=M[48]
    // M[M[72]+8]=M[64]
    int var8 = M[32+rsp];           // rbx
    int var9 = rsp+40;              // rax
    int var10 = rsp+80;             // rsi
    int var11 = var8;               // rcx
    int var12;
    for( ; var9!=var10; var9 += 8){
        var12 = M[var9];            // rdx
        M[var11+8] = var12;
        var11 = var12;
    }
    // 581-591
    // M[M[32]] >= M[M[M[32]+8]] == M[M[40]]
    // M[M[40]] >= M[M[48]]
    // M[M[72]] >= M[M[64]]
    // 注意只比低4字节
    M[var12+8]=0;
    for(int var13 = 5; var13 != 0; var13--){
        int var14 = M[var8+8];
        var14 = M[var14];
        if(M[var8] < var14){
            printf("Bomb!!!");
                return;
        }
        var8 = M[var8+8];
    }
}


int main(int argc, char ** argv){

} 