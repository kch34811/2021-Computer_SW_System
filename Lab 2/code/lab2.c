/* CSED 211 Fall '2021.  Lab L2 */

#if 0
FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict compared to the previous homework.  
You are allowed to use looping and conditional control.  
You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.

#endif

/* 
 * float_neg - Return bit-level equivalent of expression -f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   When argument is NaN, return argument.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Rating: 2
 */
unsigned float_neg(unsigned uf) {
     // frac part is 111111...11
      int frac_mask = (1 << 24) - 1;

     //exp part is 11111111
     int exp_mask = 0xFF << 23;

     //if it's NaN, return uf
     if (((uf & exp_mask) == exp_mask) && (uf & frac_mask))
          return uf;

     //if it's not NaN, change sign bit
     return uf ^ (1 << 31);
}
/* 
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Rating: 4
 */
unsigned float_i2f(int x) {

    //temporary variance for s, exp, frac
    int s_temp = 0;
    int exp_temp = 158;
    int frac_temp = x;

    // x is 000...00
    if (x == 0)
        return 0;
   
    // x is 1000...00
    int MSB = 1 << 31;
    if (x == MSB)
        return (MSB | (exp_temp << 23));

    // even if x is negative, change to positive
    if (x < 0) {
        s_temp = 1;
        frac_temp = -x;
    }

    // calculate for exp and frac
    while (!(frac_temp & MSB)) {
        frac_temp <<= 1;
        exp_temp -= 1;
    }

    //find s, exp, frac
    int s = 0;
    if (s_temp == 1)
        s = MSB;
    int exp = exp_temp << 23;
    int frac = (frac_temp & ~(1 << 31)) >> 8;

    //rounding
    int rounding = frac_temp & ((1 << 8) - 1);
    if ((rounding > 128) || ((rounding == 128) && (frac & 1)))
        frac += 1;

    //result
    return s + exp + frac;
}
/* 
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
    //for NaN
    int exp_mask = 0xFF << 23;
    int frac_mask = (1 << 24) - 1;
    if (((uf & exp_mask) == exp_mask) && (uf & frac_mask))
        return uf;

    //for infinity, -infinity, 0, -0
    if ((uf & exp_mask) == exp_mask || (uf == 0) || (uf == (1 << 31)))
        return uf;

    //for denormalized numbers
    if ((uf & exp_mask) == 0)
        return uf + (uf & frac_mask);

    //for normalized numbers
    return uf + (1 << 23);
}

/* 
 * float_abs - Return bit-level equivalent of absolute value of f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   When argument is NaN, return argument.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Rating: 2
 */
unsigned float_abs(unsigned uf) {
    //for NaN
    int exp_mask = 0xFF << 23;
    int frac_mask = (1 << 24) - 1;
    if (((uf & exp_mask) == exp_mask) && (uf & frac_mask))
        return uf;

    //when sign bit is 0, f is positive
    if ((uf >> 31) == 0)
        return uf;

    //when sign bit is 1, f is negative. so, return -f
    if ((uf >> 31) & 1)
        return uf ^ (1 << 31);
}
/* 
 * float_half - Return bit-level equivalent of expression 0.5*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Rating: 4
 */
unsigned float_half(unsigned uf) {
    //for NaN
    int exp_mask = 0xFF << 23;
    int frac_mask = (1 << 24) - 1;
    if (((uf & exp_mask) == exp_mask) && (uf & frac_mask))
        return uf;
    
    //for infinity, -infinity, 0, -0
    if ((uf & exp_mask) == exp_mask || (uf == 0) || (uf == (1 << 31)))
        return uf;

    //for denormalized numbers
    if ((uf & exp_mask) == 0)
        return uf - ((uf & frac_mask) >> 1);
    
    //for normalized numbers
    return uf - (1 << 23);
}
