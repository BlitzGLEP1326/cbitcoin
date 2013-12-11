//
//  CBVarInt.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/04/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBVarInt.h"

CBVarInt CBVarIntDecode(CBByteArray * bytes, uint32_t offset){
	return CBVarIntDecodeData(CBByteArrayGetData(bytes), offset);
}
CBVarInt CBVarIntDecodeData(uint8_t * bytes, uint32_t offset){
	CBVarInt result;
	if (bytes[offset] < 253) {
		// 8 bits.
		result.val = bytes[offset];
		result.size = 1;
	} else if (bytes[offset] == 253) {
		// 16 bits.
		result.val = CBArrayToInt16(bytes, offset + 1);
		result.size = 3;
	} else if (bytes[offset] == 254) {
		// 32 bits.
		result.val = CBArrayToInt32(bytes, offset + 1);
		result.size = 5;
	} else {
		// 64 bits.
		result.val = CBArrayToInt64(bytes, offset + 1);
		result.size = 9;
	}
	return result;
}
void CBVarIntEncode(CBByteArray * bytes, uint32_t offset, CBVarInt varInt){
	switch (varInt.size) {
		case 1:
			CBByteArraySetByte(bytes, offset, (uint8_t)varInt.val);
			break;
		case 3:
			CBByteArraySetByte(bytes, offset, 253);
			CBByteArraySetInt16(bytes, offset + 1, (uint16_t)varInt.val);
			break;
		case 5:
			CBByteArraySetByte(bytes, offset, 254);
			CBByteArraySetInt32(bytes, offset + 1, (uint32_t)varInt.val);
			break;
		case 9:
			CBByteArraySetByte(bytes, offset, 255);
			CBByteArraySetInt64(bytes, offset + 1, varInt.val);
			break;
	}
}
CBVarInt CBVarIntFromUInt64(uint64_t integer){
	CBVarInt varInt;
	varInt.val = integer;
	varInt.size = CBVarIntSizeOf(integer);
	return varInt;
}
uint8_t CBVarIntSizeOf(uint64_t value){
	if (value < 253)
		return 1;
	else if (value < 65536)
		return 3;  // 1 marker + 2 data bytes
	else if (value < 4294967296)
		return 5;  // 1 marker + 4 data bytes
	return 9; // 1 marker + 4 data bytes
}

