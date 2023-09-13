#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void addrToBinary(char * start, int len){
    char *res = (char*)malloc(16);
    for(int i = 0; i < 16; i++){
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
    printf("10d%s: ", start);
    for(int i = 11; i >= 0; i--){
        if(i == 4 || i == 9){
            printf(",");
        }
        printf("%c", res[i]);
    }
    printf("\n\n");
    free(res);
}

int main(){
    int len = 3;
    char * str1 = "0a0";
    addrToBinary(str1, len);
    str1 = "0c0";
    addrToBinary(str1, len);
    str1 = "0e0";
    addrToBinary(str1, len);
    str1 = "110";
    addrToBinary(str1, len);
    str1 = "130";
    addrToBinary(str1, len);
    str1 = "150";
    addrToBinary(str1, len);
    str1 = "170";
    addrToBinary(str1, len);
    str1 = "190";
    addrToBinary(str1, len);
    str1 = "1a0";
    addrToBinary(str1, len);
    str1 = "1c0";
    addrToBinary(str1, len);
    str1 = "1e0";
    addrToBinary(str1, len);
    str1 = "210";
    addrToBinary(str1, len);
    str1 = "230";
    addrToBinary(str1, len);
    str1 = "250";
    addrToBinary(str1, len);
    str1 = "270";
    addrToBinary(str1, len);
    str1 = "290";
    addrToBinary(str1, len);

}