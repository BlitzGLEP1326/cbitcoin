//
//  CBVersionChecksumBytes.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/05/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBVersionChecksumBytes.h"

//  Constructors

CBVersionChecksumBytes * CBNewVersionChecksumBytesFromString(CBByteArray * string, bool cacheString){
	CBVersionChecksumBytes * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeVersionChecksumBytes;
	if(CBInitVersionChecksumBytesFromString(self, string, cacheString))
		return self;
	free(self);
	return NULL;
}
CBVersionChecksumBytes * CBNewVersionChecksumBytesFromBytes(uint8_t * bytes, uint32_t size, bool cacheString){
	CBVersionChecksumBytes * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeVersionChecksumBytes;
	CBInitVersionChecksumBytesFromBytes(self, bytes, size, cacheString);
	return self;
}

//  Object Getter

CBVersionChecksumBytes * CBGetVersionChecksumBytes(void * self){
	return self;
}

//  Initialisers

bool CBInitVersionChecksumBytesFromString(CBVersionChecksumBytes * self, CBByteArray * string, bool cacheString){
	// Cache string if needed
	if (cacheString) {
		self->cachedString = string;
		CBRetainObject(string);
	}else
		self->cachedString = NULL;
	self->cacheString = cacheString;
	// Get bytes from string conversion
	CBBigInt bytes;
	CBBigIntAlloc(&bytes, 25); // 25 is the number of bytes for bitcoin addresses.
	if (NOT CBDecodeBase58Checked(&bytes, (char *)CBByteArrayGetData(string)))
		return false;
	// Take over the bytes with the CBByteArray
	CBInitByteArrayWithData(CBGetByteArray(self), bytes.data, bytes.length);
	CBByteArrayReverseBytes(CBGetByteArray(self)); // CBBigInt is in little-endian. Conversion needed to make bitcoin address the right way.
	return true;
}
void CBInitVersionChecksumBytesFromBytes(CBVersionChecksumBytes * self, uint8_t * bytes, uint32_t size, bool cacheString){
	self->cacheString = cacheString;
	self->cachedString = NULL;
	CBInitByteArrayWithData(CBGetByteArray(self), bytes, size);
}

//  Destructor

void CBDestroyVersionChecksumBytes(void * vself){
	CBVersionChecksumBytes * self = vself;
	if (self->cachedString) CBReleaseObject(self->cachedString);
	CBFreeByteArray(CBGetByteArray(self));
}
void CBFreeVersionChecksumBytes(void * self){
	CBDestroyVersionChecksumBytes(self);
	free(self);
}

//  Functions

uint8_t CBVersionChecksumBytesGetVersion(CBVersionChecksumBytes * self){
	return CBByteArrayGetByte(CBGetByteArray(self), 0);
}
CBByteArray * CBVersionChecksumBytesGetString(CBVersionChecksumBytes * self){
	if (self->cachedString) {
		// Return cached string
		CBRetainObject(self->cachedString);
		return self->cachedString;
	}else{
		// Make string
		CBByteArrayReverseBytes(CBGetByteArray(self)); // Make this into little-endian
		CBBigInt bytes;
		CBBigIntAlloc(&bytes, CBGetByteArray(self)->length);
		bytes.length = CBGetByteArray(self)->length;
		memcpy(bytes.data, CBByteArrayGetData(CBGetByteArray(self)), bytes.length);
		char * string = CBEncodeBase58(&bytes);
		CBByteArray * str = CBNewByteArrayFromString(string, true);
		CBByteArrayReverseBytes(CBGetByteArray(self)); // Now the string is got, back to big-endian.
		if (self->cacheString) {
			self->cachedString = str;
			CBRetainObject(str); // Retain for this object.
		}
		return str; // No additional retain. Retained from constructor.
	}
}
