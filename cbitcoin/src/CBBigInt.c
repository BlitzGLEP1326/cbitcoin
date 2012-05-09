//
//  CBBigInt.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/04/2012.
//  Last modified by Matthew Mitchell on 10/05/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin.
//
//  cbitcoin is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  
//  cbitcoin is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with cbitcoin.  If not, see <http://www.gnu.org/licenses/>.
//
//  Contains BIGDIGITS multiple-precision arithmetic code originally
//  written by David Ireland, copyright (c) 2001-11 by D.I. Management
//  Services Pty Limited <www.di-mgt.com.au>, and is used with
//  permission.

#include "CBBigInt.h"

u_int8_t CBPowerOf2Log2(u_int8_t a){
	switch (a) {
		case 1:
			return 0;
		case 2:
			return 1;
		case 4:
			return 2;
		case 8:
			return 3;
		case 16:
			return 4;
		case 32:
			return 5;
		case 64:
			return 6;
	}
	return 7;
}
u_int8_t CBFloorLog2(u_int8_t a){
	if (a < 16){
		if (a < 4) {
			if (a == 1){
				return 0;
			}
			return 1;
		}
		if (a < 8){
			return 2;
		}
		return 3;
	}
	if (a < 64){
		if (a < 32) {
			return 4;
		}
		return 5;
	}
	if (a < 128){
		return 6;
	}
	return 7;
}
CBCompare CBBigIntCompare(CBBigInt a,CBBigInt b){
	u_int16_t len;
	if (a.length > b.length){
		return CB_COMPARE_MORE_THAN;
	}else if (a.length < b.length){
		return CB_COMPARE_LESS_THAN;
	}else
		len = a.length;
	for (u_int16_t x = 0;x < len; x++) {
		if (a.data[x] < b.data[x])
			return CB_COMPARE_LESS_THAN;
		else if (a.data[x] > b.data[x])
			return CB_COMPARE_MORE_THAN;
	}
	return CB_COMPARE_EQUAL;
}
CBCompare CBBigIntCompareToUInt8(CBBigInt a,u_int8_t b){
	int x = 0;
	for (; x < a.length - 1; x++)
		if (a.data[x]) 
			return CB_COMPARE_MORE_THAN;
	if (a.data[x] > b)
		return CB_COMPARE_MORE_THAN;
	else if (a.data[x] < b)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
}
void CBBigIntEqualsDivisionByUInt8(CBBigInt * a,u_int8_t b,u_int8_t * ans){
	if (!(b & (b - 1))){
		// For powers of two, division can be done through bit shifts.
		CBBigIntEqualsRightShiftByUInt8(a,CBPowerOf2Log2(b));
		return;
	}
	// Determine how many times b will fit into a as a power of two and repeat for the remainders
	u_int8_t begin = 0; // Begining of CBBigInt in calculations
	bool continuing = true;
	u_int8_t leftMost;
	bool first = true;
	while (continuing){
		// How much does b have to be shifted by before it becomes larger than a? Complete the shift into a shiftedByte
		int16_t shiftAmount;
		u_int16_t shiftedByte;
		if (a->data[begin] > b){
			shiftAmount = CBFloorLog2(a->data[begin]/b);
			shiftedByte = b << 8 + shiftAmount;
		}else if (a->data[begin] < b){
			shiftAmount = -CBFloorLog2(b/a->data[begin]);
			shiftedByte = b << 8 + shiftAmount;
			// Shift right once again if "shiftedByte > (a->data[begin] << 8) + a->data[begin+1]" as the shifted divisor should be smaller
			if (shiftedByte > ((a->data[begin] << 8) + a->data[begin+1])){
				shiftedByte >>= 1;
				shiftAmount--; // Do not forget about changing "shiftAmount" for calculations
			}
		}else{
			shiftAmount = 0;
			shiftedByte = b << 8;
		}
		// Set bit on "ans"
		if (shiftAmount < 0){ // If "shiftAmount" is negative then the byte moves right.
			ans[begin+1] |= 1 << (8 + shiftAmount);
			if (first) leftMost = 1;
		}else{
			ans[begin] |= 1 << shiftAmount; // No movement to right byte, jsut shift bit into place.
			if (first) leftMost = 0;
		}
		first = false; // Do not set "leftMost" from here on
		// Take away the shifted byte to give the remainder
		u_int16_t sub = (a->data[begin] << 8) + a->data[begin+1] - shiftedByte;
		a->data[begin] = sub >> 8;
		if (begin != a->length - 1) 
			a->data[begin + 1] = sub; // Move second byte into next data byte if exists.
		// Move along "begin" to byte with more data
		for (u_int8_t x = begin;; x++){
			if (a->data[x]){
				if (x == a->length - 1)
					// Last byte
					if (a->data[x] < b){
						// b can fit no more
						continuing = false;
						break;
					}
				begin = x;
				break;
			}
			if (x == a->length - 1){
				continuing = false; // No more data
				break;
			}
		}
	}
	a->length -= leftMost; // If the first bit was onto the 2nd byte then the length is less one
	memmove(a->data, ans + leftMost, a->length); // Done calculation. Move ans to "a".
}
void CBBigIntEqualsRightShiftByUInt8(CBBigInt * a,u_int8_t b){
	u_int8_t deadBytes = b / 8; // These bytes fall off the side.
	a->length -= deadBytes; // Reduce length of bignum by the removed bytes
	u_int8_t remainderShift = b % 8;
	if (!remainderShift) { // No more work
		return;
	}
	u_int16_t splitter;
	u_int8_t toRight = 0; // Bits taken from the left to the next byte.
	for (u_int8_t x = 0; x < a->length; x++) {
		splitter = a->data[x] << 8 - remainderShift; // Splits data in splitters between first and second byte.
		a->data[x] = splitter >> 8; // First byte in splitter is the new data.
		a->data[x] |= toRight; // Take the bits from the left
		toRight = splitter; // Second byte is the data going to the right from this byte.
	}
}
void CBBigIntEqualsSubtractionByUInt8(CBBigInt * a,u_int8_t b){
	u_int16_t end = a->data[a->length-1] + (a->data[a->length-2] << 8);
	end -= b;
	a->data[a->length-1] = end;
	a->data[a->length-2] = end >> 8;
}
u_int8_t CBBigIntModuloWithUInt8(CBBigInt a,u_int8_t b){
	if (!(b & (b - 1)))
		return a.data[a.length - 1] & b - 1; // For powers of two this can be done
	// Wasn't a power of two. Use method presented here: http://stackoverflow.com/a/10441333/238411
	u_int16_t result = 0; // Prevents overflow in calculations
	for(u_int8_t x = 0; x < a.length; x++){
		result *= (256 % b);
		result %= b;
		result += a.data[x] % b;
		result %= b;
	}
	return result;
}
void CBBigIntNormalise(CBBigInt * a){
	for (int x = 0; x < a->length; x++){
		if (a->data[x]){
			if (x == 0)
				break;
			a->data += x;
			a->length -= x;
			break;
		}else if (x == a->length - 1){
			// End with zero
			a->data += x;
			a->length -= x;
		}
	}
}
