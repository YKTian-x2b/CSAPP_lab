#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// 就是看[0, 14]里哪个数能满足下面这个递归
int fun4(int arg1, int arg2, int arg3){
    int var1 = arg3 - arg2;
    // 模拟逻辑右移的结果
    int x = 1 << 31;
    int y = (x & var1) >> 31;
    int var2 = y;
    // int var2 = var1 >> 31;  // 逻辑
    var1 += var2;
    var1 >>= 1;             // 算术
    int var3 = arg2+var1;
    if(var3 <= arg1){
        int res = 0;
        if(var3 >= arg1){
            return res;
        }
        else{
            res = fun4(arg1, var3 + 1, arg3);
            return res*2+1;
        }
    }
    else{
        int res2 = fun4(arg1, arg2, var3-1);
        return res2*2;
    }
}
// 答案是 0 1 3 7
int main(int argc, char ** argv){
    for(int i = 0; i <= 14; i++){
        printf("%d ", fun4(i, 0, 14));
    }
} 