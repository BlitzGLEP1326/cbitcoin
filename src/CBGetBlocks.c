//
//  CBGetBlocks.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 11/07/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION 

#include "CBGetBlocks.h"

//  Constructors

CBGetBlocks * CBNewGetBlocks(uint32_t version, CBChainDescriptor * chainDescriptor, CBByteArray * stopAtHash){
	CBGetBlocks * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeGetBlocks;
	CBInitGetBlocks(self, version, chainDescriptor, stopAtHash);
	return self;
}
CBGetBlocks * CBNewGetBlocksFromData(CBByteArray * data){
	CBGetBlocks * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeGetBlocks;
	CBInitGetBlocksFromData(self, data);
	return self;
}

//  Object Getter

CBGetBlocks * CBGetGetBlocks(void * self){
	return self;
}

//  Initialisers

void CBInitGetBlocks(CBGetBlocks * self, uint32_t version, CBChainDescriptor * chainDescriptor, CBByteArray * stopAtHash){
	self->version = version;
	self->chainDescriptor = chainDescriptor;
	CBRetainObject(chainDescriptor);
	self->stopAtHash = stopAtHash;
	CBRetainObject(stopAtHash);
	CBInitMessageByObject(CBGetMessage(self));
}
void CBInitGetBlocksFromData(CBGetBlocks * self, CBByteArray * data){
	self->chainDescriptor = NULL;
	self->stopAtHash = NULL;
	CBInitMessageByData(CBGetMessage(self), data);
}

//  Destructor

void CBDestroyGetBlocks(void * vself){
	CBGetBlocks * self = vself;
	if (self->chainDescriptor) CBReleaseObject(self->chainDescriptor);
	if (self->stopAtHash) CBReleaseObject(self->stopAtHash);
	CBDestroyMessage(self);
}
void CBFreeGetBlocks(void * self){
	CBDestroyGetBlocks(self);
	free(self);
}

//  Functions

uint16_t CBGetBlocksDeserialise(CBGetBlocks * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBLogError("Attempting to deserialise a CBGetBlocks with no bytes.");
		return 0;
	}
	if (bytes->length < 69) { // 4 version bytes, 33 chain descriptor bytes, 32 stop at hash bytes
		CBLogError("Attempting to deserialise a CBGetBlocks with less bytes than required for one hash.");
		return 0;
	}
	self->version = CBByteArrayReadInt32(bytes, 0);
	// Deserialise the CBChainDescriptor
	CBByteArray * data = CBByteArraySubReference(bytes, 4, bytes->length-4);
	self->chainDescriptor = CBNewChainDescriptorFromData(data);
	uint16_t len = CBChainDescriptorDeserialise(self->chainDescriptor);
	if (NOT len){
		CBLogError("CBGetBlocks cannot be deserialised because of an error with the CBChainDescriptor.");
		CBReleaseObject(data);
		return 0;
	}
	data->length = len; // Re-adjust length for the chain descriptor object's reference.
	CBReleaseObject(data); // Release this reference.
	// Deserialise stopAtHash
	if (bytes->length < len + 36) { // The chain descriptor length plus the version and stopAtHash bytes.
		CBLogError("Attempting to deserialise a CBGetBlocks with less bytes than required for the stopAtHash.");
		return 0;
	}
	self->stopAtHash = CBNewByteArraySubReference(bytes, len + 4, 32);
	if (NOT self->stopAtHash) {
		CBLogError("Cannot create new CBByteArray in CBGetBlocksDeserialise\n");
		CBReleaseObject(self->chainDescriptor);
	}
	return len + 36;
}
uint32_t CBGetBlocksCalculateLength(CBGetBlocks * self){
	return 36 + self->chainDescriptor->hashNum * 32 + CBVarIntSizeOf(self->chainDescriptor->hashNum);
}
uint16_t CBGetBlocksSerialise(CBGetBlocks * self, bool force){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBLogError("Attempting to serialise a CBGetBlocks with no bytes.");
		return 0;
	}
	if (bytes->length < 36 + self->chainDescriptor->hashNum * 32 + CBVarIntSizeOf(self->chainDescriptor->hashNum)) {
		CBLogError("Attempting to serialise a CBGetBlocks with less bytes than required.");
		return 0;
	}
	CBByteArraySetInt32(bytes, 0, self->version);
	// Serialise chain descriptor
	uint32_t len;
	if (NOT CBGetMessage(self->chainDescriptor)->serialised // Serailise if not serialised yet.
		// Serialise if force is true.
		|| force
		// If the data shares the same data as the get blocks message, re-serialise the chain descriptor, in case it got overwritten.
		|| CBGetMessage(self->chainDescriptor)->bytes->sharedData == bytes->sharedData) {
		if (CBGetMessage(self->chainDescriptor)->serialised)
			// Release old byte array
			CBReleaseObject(CBGetMessage(self->chainDescriptor)->bytes);
		CBGetMessage(self->chainDescriptor)->bytes = CBByteArraySubReference(bytes, 4, bytes->length-4);
		if (NOT CBGetMessage(self->chainDescriptor)->bytes) {
			CBLogError("Cannot create a new CBByteArray sub reference in CBGetBlocksSerialise");
			return 0;
		}
		len = CBChainDescriptorSerialise(self->chainDescriptor);
		if (NOT len) {
			CBLogError("CBGetBlocks cannot be serialised because of an error with the chain descriptor. This error should never occur... :o");
			// Release bytes to avoid problems overwritting pointer without release, if serialisation is tried again.
			CBReleaseObject(CBGetMessage(self->chainDescriptor)->bytes);
			return 0;
		}
	}else{
		// Move serialsed data to one location
		CBByteArrayCopyByteArray(bytes, 4, CBGetMessage(self->chainDescriptor)->bytes);
		CBByteArrayChangeReference(CBGetMessage(self->chainDescriptor)->bytes, bytes, 4);
		len = CBGetMessage(self->chainDescriptor)->bytes->length;
	}
	// Serialise stopAtHash
	CBByteArrayCopyByteArray(bytes, len + 4, self->stopAtHash);
	CBByteArrayChangeReference(self->stopAtHash, bytes, len + 4);
	// Ensure length is correct
	bytes->length = len + 36;
	// Is now serialised.
	CBGetMessage(self)->serialised = true;
	return len + 36;
}

