/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
  // x和y同为0或同为1的bit取0 其他位取1
  return ~(x&y) & ~(~x & ~y);
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  return 64 << 25;
}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) {
  // tmax+1 == ~tmax == tmin
  int tmin_a = x+1;
  int tmin_b = ~x;
  return !(tmin_a ^ tmin_b) & !!(~x);
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
  // 掩码筛选出奇数位 再同掩码对比
  int stand = (((((170<<8)+170)<<8)+170)<<8)+170;
  int and = stand & x;
  return !(and ^ stand);
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return ~x+1;
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
  // 在x是正数的情况下，对x的高28位而言，能且只能 低2位是1；对x的低4位而言，必须小于等于9，减10就一定是负数
  int flag1 = x >> 31;
  int tmin = 64 << 25;
  int flag2 = (~10+1) + ((3<<4)^x);
  flag2 = flag2 & tmin;
  return !(flag1 | !flag2);
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
  // (1-k)*y + k*z, k=0/1
  return ((~(!!x)+1)&y) + ((~(!x)+1)&z);
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  // 先验知识 -x = (~x)+1 不适用于int_min; 要小心加法溢出，所以直接分情况讨论
  int tmin = 64<<25;
  int y_is_tmin = !(y ^ tmin);
  int x_is_tmin = !(x ^ tmin);
  int x_isNeg = x >> 31 & 1;
  int y_isNeg = y >> 31 & 1;
  int is_not_same = x_isNeg ^ y_isNeg;
  int xminusyplus1 = (~y)+x;
  int flag = xminusyplus1 >> 31 & 1;
  // 如果两个数符号相同，那么 如果减法的结果小于等于0，则成立；等价的，再减1小于0即可。（肯定减不出tmin）
  int condi_same = !is_not_same & ((y_is_tmin & x_is_tmin) | (!y_is_tmin & flag));
  // 如果两个数符号不同，则大小可以从符号位比出来
  int condi_not_same = is_not_same & x_isNeg;
  return  condi_same | condi_not_same;
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
  // 只有tmin和0的~x+1 是本身，那符号位就是 00 01 11，要00
  int flag1 = x>>31 & 1;
  int flag2 = (~x+1)>>31 & 1;
  int flag3 = flag1 | flag2;
  return 2 + ~flag3;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
  // 1.如果是负数，直接取反,统一成正数进行处理。每一个负数和其取反得到的正数需要的表示位数一样。
  int x_isNeg = x >> 31 & 1;
  int new_x = ((~x_isNeg+1) & (~x)) | ((~(!x_isNeg)+1) & x);
  // 2.如果高16位为全0，则只需要判断低16位需要的位数。同样，如果高16位不全为0，则至少需要16+1位。
  int high = new_x >> 16;
  int low = new_x & ((255<<8)+255);
  int high_not_act = !high;
  int res_part = (~(!high_not_act)+1) & 16;
  // 3.对于新的16bit挨个判断01
  int new_new_x = ((~(!high_not_act)+1) & high) | ((~high_not_act+1) & low);
  // 这里，任意数都至少需要一个bit来表示，所以直接略过最低位。我们要找到最高位1的所在位置，然后该位置再往高一位就是所需的位数。（因为是正数，所以需要符号位0）
  // 对于高15bit，如果前15-n位（n为迭代计数）不全为零，则没有找到最高位1，那么我们需要给forward_res加1, 右移一次。直到走过了所有1。
  // 之后，所有的高位都是0,我们就给forward_res加0。（这里要加的0/1，直接用当前nw转换形成,即，!!nw==当前的0/1）
  int nw0 = new_new_x >> 1;
  // 但是15次!!过于占用符号配额，我们必须去掉一个！。以此思路，倒推，本来记1的情况，我们记成0，然后用16一减就OK。
  // 或者，计数的思路改变，我们记前15-n位（n为迭代计数）全为零的情况。
  int res0 = 0;
  // 15次右移计数
  int res1 = res0 + !nw0;
  int nw1 = nw0 >> 1;
  int res2 = res1 + !nw1;
  int nw2 = nw1 >> 1;
  int res3 = res2 + !nw2;
  int nw3 = nw2 >> 1;
  int res4 = res3 + !nw3;
  int nw4 = nw3 >> 1;
  int res5 = res4 + !nw4;
  int nw5 = nw4 >> 1;
  int res6 = res5 + !nw5;
  int nw6 = nw5 >> 1;
  int res7 = res6 + !nw6;
  int nw7 = nw6 >> 1;
  int res8 = res7 + !nw7;
  int nw8 = nw7 >> 1;
  int res9 = res8 + !nw8;
  int nw9 = nw8 >> 1;
  int res10 = res9 + !nw9;
  int nw10 = nw9 >> 1;
  int res11 = res10 + !nw10;
  int nw11 = nw10 >> 1;
  int res12 = res11 + !nw11;
  int nw12 = nw11 >> 1;
  int res13 = res12 + !nw12;
  int nw13 = nw12 >> 1;
  int res14 = res13 + !nw13;
  int nw14 = nw13 >> 1;
  int res15 = res14 + !nw14;
  // 这里，刚刚的思路有个漏洞，我们遇到最高位1时，会默认加一位符号位。但是，如果一位1都没有，0x0的情况下，我们就多加了一个。
  // 所以，特判，如果是0x0,直接再减一个。
  int tp = (~(!new_new_x)+1) & (~0);
  // res_part + 15 - res15 + 1 + 1 + tp (高低16位+15次计数+符号位+最低位+特判)
  return  res_part + 18 + (~res15)  + tp;
}
//float
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf) {
  unsigned mask_e = 255<<23;
  unsigned s = uf & (64<<25);
  unsigned e = uf & mask_e;
  unsigned f = uf & 8388607;
  unsigned new_f, new_e;
  // 对于无穷和NaN,*2还是原数据
  if(e == mask_e){
    return uf;
  }
  else if(e == 0){
    // f*2 == f<<1
    // 对于非规格浮点数，0.x * 2^(1-Bias) 的二倍 要么是 0.(x<<1) * 2^(1-Bias);要么是 1.(x<<1) * 2^(1-Bias)
    // 或者 如果尾数溢出，则正好进入规格浮点数的领域，丝滑过渡；如果没溢出，则简单左移；
    new_f = f << 1;
    new_e = e;
    if(new_f > 8388607){
      new_e = e+(1<<23);
      new_f = new_f - 8388608;
    }
  }
  else{
    // 对于规格浮点数，1.x * 2^(e-Bias) 的二倍 就是 1.x * 2^(e+1-Bias)
    new_e = e+(1<<23);
    new_f = f;
    if(new_e == mask_e){
      new_f = 0;
    }
  }
  return s+new_e+new_f;
}

/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf) {
  unsigned tmin = 64<<25;
  unsigned mask_e = 255<<23;
  unsigned mask_f_highest_bit = 1 << 22;
  unsigned s = uf & tmin;
  unsigned e = uf & mask_e;
  unsigned f = uf & 8388607;

  unsigned res = 1;
  int count = (e>>23) - 127;

  // 对于无穷和NaN,返回tmin;
  if(e == mask_e){
    return tmin;
  }
  else if(e == 0 || count < 0){
    // 对于非规格浮点数，直接返回0
    // 对于规格浮点数 1.x * 2^(-y)，仍旧返回0
    return 0;
  }
  else{
    // 对于规格浮点数 1.x * 2^(y)，尾数每次左移进一位，直到溢出返回tmin
    while(count > 0){
      res = (res << 1) + ((f & mask_f_highest_bit) >> 22);
      f = f << 1;
      count = count - 1;  

      if(!(res ^ tmin)) return tmin;
    }

    if(s == tmin){
      return ~res+1;
    }
    return res;
  }
}
/* 
 * floatPower2 - Return bit-level equivalent of precision floating point value.
 *   Anything out of range (including NaN and infinity) should returner x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
unsigned floatPower2(int x) {
  if(x < -126){
    return 0;
  }
  else if(x > 127){
    return 255 << 23;
  }
  else {
    int new_x = x + 127;
    return new_x << 23;
  }
}
