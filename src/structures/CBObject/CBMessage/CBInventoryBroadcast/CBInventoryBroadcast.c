//
//  CBInventoryBroadcast.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 10/07/2012.
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

//  SEE HEADER FILE FOR DOCUMENTATION 

#include "CBInventoryBroadcast.h"

//  Constructors

CBInventoryBroadcast * CBNewInventoryBroadcast(CBEvents * events){
	CBInventoryBroadcast * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeInventoryBroadcast;
	CBInitInventoryBroadcast(self,events);
	return self;
}
CBInventoryBroadcast * CBNewInventoryBroadcastFromData(CBByteArray * data,CBEvents * events){
	CBInventoryBroadcast * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeInventoryBroadcast;
	CBInitInventoryBroadcastFromData(self,data,events);
	return self;
}

//  Object Getter

CBInventoryBroadcast * CBGetInventoryBroadcast(void * self){
	return self;
}

//  Initialisers

bool CBInitInventoryBroadcast(CBInventoryBroadcast * self,CBEvents * events){
	self->itemNum = 0;
	self->items = NULL;
	if (!CBInitMessageByObject(CBGetMessage(self), events))
		return false;
	return true;
}
bool CBInitInventoryBroadcastFromData(CBInventoryBroadcast * self,CBByteArray * data,CBEvents * events){
	self->itemNum = 0;
	self->items = NULL;
	if (!CBInitMessageByData(CBGetMessage(self), data, events))
		return false;
	return true;
}

//  Destructor

void CBFreeInventoryBroadcast(void * vself){
	CBInventoryBroadcast * self = vself;
	for (u_int16_t x = 0; x < self->itemNum; x++) {
		CBReleaseObject(&self->items[x]); // Free item
	}
	free(self->items); // Free item pointer memory block.
	CBFreeMessage(self);
}

//  Functions

u_int32_t CBInventoryBroadcastDeserialise(CBInventoryBroadcast * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBInventoryBroadcast with no bytes.");
		return 0;
	}
	if (bytes->length < 37) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBInventoryBroadcast with less bytes than required for one item.");
		return 0;
	}
	CBVarInt itemNum = CBVarIntDecode(bytes, 0);
	if (itemNum.val > 1388) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBInventoryBroadcast with a var int over 1388.");
		return 0;
	}
	// Run through the items and deserialise each one.
	self->items = malloc(sizeof(*self->items) * (size_t)itemNum.val);
	self->itemNum = itemNum.val;
	u_int16_t cursor = itemNum.size;
	for (u_int16_t x = 0; x < itemNum.val; x++) {
		// Make new CBInventoryItem from the rest of the data.
		CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		self->items[x] = CBNewInventoryItemFromData(data, CBGetMessage(self)->events);
		// Deserialise
		u_int8_t len = CBInventoryItemDeserialise(self->items[x]);
		if (!len) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBInventoryBroadcast cannot be deserialised because of an error with the CBInventoryItem number %u.",x);
			// Release bytes
			CBReleaseObject(&data);
			// Because of failure release the CBInventoryItems
			for (u_int16_t y = 0; y < x + 1; y++) {
				CBReleaseObject(&self->items[y]);
			}
			free(self->items);
			self->items = NULL;
			self->itemNum = 0;
			return 0;
		}
		// Adjust length
		data->length = len;
		CBReleaseObject(&data);
		cursor += len;
	}
	return cursor;
}
u_int32_t CBInventoryBroadcastSerialise(CBInventoryBroadcast * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBInventoryBroadcast with no bytes.");
		return 0;
	}
	if (bytes->length < 36 * self->itemNum) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBInventoryBroadcast with less bytes than required.");
		return 0;
	}
	CBVarInt num = CBVarIntFromUInt64(self->itemNum);
	CBVarIntEncode(bytes, 0, num);
	u_int16_t cursor = num.size;
	for (u_int16_t x = 0; x < num.val; x++) {
		CBGetMessage(self->items[x])->bytes = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		u_int32_t len = CBInventoryItemSerialise(self->items[x]);
		if (!len) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"CBInventoryBroadcast cannot be serialised because of an error with the CBInventoryItem number %u.",x);
			// Release CBByteArray objects to avoid problems overwritting pointer without release, if serialisation is tried again.
			for (u_int8_t y = 0; y < x + 1; y++) {
				CBReleaseObject(&CBGetMessage(self->items[y])->bytes);
			}
			return 0;
		}
		CBGetMessage(self->items[x])->bytes->length = len;
		cursor += len;
	}
	return cursor;
}

