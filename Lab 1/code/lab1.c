/* CSED 211 Fall '2021.  Lab L1 */

#if 0
LAB L1 INSTRUCTIONS :

#endif

/*
 * bitAnd - x&y using only ~ and |
 *   Example: bitAnd(6, 5) = 4
 *   Legal ops: ~ |
 */
int bitAnd(int x, int y) {
	return ~(~x | ~y);
}

/*
 * addOK - Determine if can compute x+y without overflow
 *   Example: addOK(0x80000000,0x80000000) = 0,
 *            addOK(0x80000000,0x70000000) = 1,
 *   Legal ops: ! ~ & ^ | + << >>
 */
int addOK(int x, int y) {
	int xMSB = (x >> 31) & 1; //negative 1, positive 0
	int yMSB = (y >> 31) & 1;
	int difMSB = xMSB ^ yMSB; //if different sign 1, if same 0
	int sumMSB = ((x + y) >> 31) & 1; //negative 1, positive 0 
	int sameSumMSB = !(sumMSB ^ xMSB); //if sum and x are same sign 1, otherwise 0
	return difMSB | sameSumMSB;
}

/*
 * isNegative - return 1 if x < 0, return 0 otherwise
 *   Example: isNegative(-1) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int isNegative(int x) {
	return (x >> 31) & 1; //if negative 1, positive 0
}

/*
 * logicalShift - logical right shift of x by y bits, 1 <= y <= 31
 *   Example: logicalShift(-1, 1) = TMax.
 *   Legal ops: ! ~ & ^ | + << >>
 */
int logicalShift(int x, int y)
{
	int leftShift = 32 + (~y + 1); //32-y
	int bitMask = ~((~0) << leftShift); //000...111 that has y 0's
	return (x >> y) & bitMask;
}


/*
 * bitCount - returns count of number of 1's in word
 *   Examples: bitCount(5) = 2, bitCount(7) = 3
 *   Legal ops: ! ~ & ^ | + << >>
 */
int bitCount(int x) {
	int bitMask = 1 | 1 << 8 | 1 << 16 | 1 << 24; //00..1 00..1 00..1 00..1
	int byteMask = 0xFF; //00..0 00..0 00..0 11..1
	int bitSum = (x & bitMask) + ((x >> 1) & bitMask) + ((x >> 2) & bitMask) + ((x >> 3) & bitMask) + ((x >> 4) & bitMask) + ((x >> 5) & bitMask) + ((x >> 6) & bitMask) + ((x >> 7) & bitMask);
	//store sum of 1's in each byte at bytes
	int byteSum = (bitSum & byteMask) + ((bitSum >> 8) & byteMask) + ((bitSum >> 16) & byteMask) + ((bitSum >> 24) & byteMask);
	// sum of 1's in bytes 
	return byteSum;
}