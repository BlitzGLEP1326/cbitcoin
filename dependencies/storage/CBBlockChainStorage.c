//
//  CBBlockChainStorage.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 20/12/2012.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBBlockChainStorage.h"

uint8_t CB_KEY_ARRAY[36];
uint8_t CB_DATA_ARRAY[22];

bool CBNewBlockChainStorage(CBDepObject * storage, char * dataDir){
	CBBlockChainStorage * self = malloc(sizeof(*self));
	if (NOT CBInitDatabase((CBDatabase *)self, dataDir, "blk")) {
		CBLogError("Could not initialise the database for a blockchain storage object.");
		free(self);
		return false;
	}
	// Load indices
	self->blockHashIndex = CBLoadIndex((CBDatabase *)self, 0, 20, 10000);
	if (self->blockHashIndex){
		self->blockIndex = CBLoadIndex((CBDatabase *)self, 1, 5, 10000);
		if (self->blockIndex){
			self->branchIndex = CBLoadIndex((CBDatabase *)self, 2, 1, 0);
			if (self->branchIndex){
				self->branchWorkIndex = CBLoadIndex((CBDatabase *)self, 3, 1, 0);
				if (self->branchWorkIndex){
					self->orphanIndex = CBLoadIndex((CBDatabase *)self, 4, 1, 0);
					if (self->orphanIndex){
						self->txIndex = CBLoadIndex((CBDatabase *)self, 5, 32, 10000);
						if (self->txIndex) {
							self->unspentOutputIndex = CBLoadIndex((CBDatabase *)self, 6, 36, 10000);
							if (self->unspentOutputIndex){
								// Create new transaction
								CBInitDatabaseTransaction(&self->tx);
								storage->ptr = self;
								return true;
							}
							CBFreeIndex(self->txIndex);
						}
						CBFreeIndex(self->orphanIndex);
					}
					CBFreeIndex(self->branchWorkIndex);
				}
				CBFreeIndex(self->branchIndex);
			}
			CBFreeIndex(self->blockIndex);
		}
		CBFreeIndex(self->blockHashIndex);
	}
	CBLogError("Could not load one of the block chain storage indices.");
	CBFreeDatabase((CBDatabase *)self);
	return false;
}
void CBFreeBlockChainStorage(CBDepObject self){
	CBBlockChainStorage * storageObj = self.ptr;
	CBFreeIndex(storageObj->blockHashIndex);
	CBFreeIndex(storageObj->blockIndex);
	CBFreeIndex(storageObj->branchIndex);
	CBFreeIndex(storageObj->branchWorkIndex);
	CBFreeIndex(storageObj->orphanIndex);
	CBFreeIndex(storageObj->txIndex);
	CBFreeIndex(storageObj->unspentOutputIndex);
	CBFreeDatabase((CBDatabase *)self.ptr);
}
bool CBBlockChainStorageBlockExists(void * validator, uint8_t * blockHash){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	memcpy(CB_KEY_ARRAY, blockHash, 20);
	return CBDatabaseGetLength((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockHashIndex, CB_KEY_ARRAY) != CB_DOESNT_EXIST;
}
bool CBBlockChainStorageChangeUnspentOutputsNum(CBBlockChainStorage * storage, uint8_t * txHash, int8_t change){
	// Place transaction hash into the key
	memcpy(CB_KEY_ARRAY, txHash, 32);
	// Read the number of unspent outputs to be decremented
	if (CBDatabaseReadValue((CBDatabase *)storage, &storage->tx, storage->txIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 4, CB_TX_REF_NUM_UNSPENT_OUTPUTS) != CB_DATABASE_INDEX_FOUND)
		return false;
	CBInt32ToArray(CB_DATA_ARRAY, 0,
				   CBArrayToInt32(CB_DATA_ARRAY, 0) + change);
	// Now write the new value
	CBDatabaseWriteValueSubSection((CBDatabase *)storage, &storage->tx, storage->txIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 4, CB_TX_REF_NUM_UNSPENT_OUTPUTS);
	return true;
}
bool CBBlockChainStorageCommitData(CBDepObject self){
	CBBlockChainStorage * storageObj = self.ptr;
	return CBDatabaseCommit((CBDatabase *)storageObj, &storageObj->tx);
}
bool CBBlockChainStorageDeleteBlock(void * validator, uint8_t branch, uint32_t blockIndex){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	// Delete from storage
	CB_KEY_ARRAY[0] = branch;
	CBInt32ToArray(CB_KEY_ARRAY, 1, blockIndex);
	// Get hash
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 20, CB_BLOCK_HASH) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not obtain a block hash from the block chain database.");
		return false;
	}
	// Remove data
	if (NOT CBDatabaseRemoveValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockIndex, CB_KEY_ARRAY)){
		CBLogError("Could not remove block value from database.");
		return false;
	}
	// Remove hash index reference
	if (NOT CBDatabaseRemoveValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockHashIndex, CB_DATA_ARRAY)){
		CBLogError("Could not remove block hash index reference from database.");
		return false;
	}
	return true;
}
bool CBBlockChainStorageDeleteUnspentOutput(void * validator, uint8_t * txHash, uint32_t outputIndex, bool decrement){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	// Place transaction hash into the key
	memcpy(CB_KEY_ARRAY, txHash, 32);
	// Place output index into the key
	CBInt32ToArray(CB_KEY_ARRAY, 32, outputIndex);
	// Remove from storage
	if (NOT CBDatabaseRemoveValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->unspentOutputIndex, CB_KEY_ARRAY)) {
		CBLogError("Could not remove an unspent output reference from storage.");
		return false;
	}
	if (decrement
		// For the transaction, decrement the number of unspent outputs
		&& NOT CBBlockChainStorageChangeUnspentOutputsNum(storageObj, txHash, -1)) {
		CBLogError("Could not decrement the number of unspent outputs for a transaction.");
		return false;
	}
	return true;
}
bool CBBlockChainStorageDeleteTransactionRef(void * validator, uint8_t * txHash){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	// Place transaction hash into the key
	memcpy(CB_KEY_ARRAY, txHash, 32);
	// Read the instance count
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->txIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 4, CB_TX_REF_INSTANCE_COUNT) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction reference from storage.");
		return false;
	}
	uint32_t txInstanceNum = CBArrayToInt32(CB_DATA_ARRAY, 0) - 1;
	if (txInstanceNum) {
		// There are still more instances of this transaction. Do not remove the transaction, only make the unspent output number equal to zero and decrement the instance count.
		CBInt32ToArray(CB_DATA_ARRAY, 0, 0);
		CBInt32ToArray(CB_DATA_ARRAY, 4, txInstanceNum);
		// Write to storage.
		CBDatabaseWriteValueSubSection((CBDatabase *)storageObj, &storageObj->tx, storageObj->txIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 8, CB_TX_REF_NUM_UNSPENT_OUTPUTS);
	// Else this was the last instance.
	// Remove from storage
	}else if (NOT CBDatabaseRemoveValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->txIndex, CB_KEY_ARRAY)) {
		CBLogError("Could not remove a transaction reference from storage.");
		return false;
	}
	return true;
}
bool CBBlockChainStorageExists(CBDepObject self){
	CBBlockChainStorage * storageObj = self.ptr;
	return CBDatabaseGetLength((CBDatabase *)storageObj, NULL, storageObj->branchIndex, (uint8_t []){CB_MAX_BRANCH_CACHE}) != CB_DOESNT_EXIST;
}
bool CBBlockChainStorageGetBlockHash(void * validator, uint8_t branch, uint32_t blockIndex, uint8_t hash[]){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	CB_KEY_ARRAY[0] = branch;
	CBInt32ToArray(CB_KEY_ARRAY, 1, blockIndex);
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockIndex, CB_KEY_ARRAY, hash, 20, CB_BLOCK_HASH) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read the hash for a block.");
		return false;
	}
	return true;
}
void * CBBlockChainStorageGetBlockHeader(void * validator, uint8_t branch, uint32_t blockIndex){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	CB_KEY_ARRAY[0] = branch;
	CBInt32ToArray(CB_KEY_ARRAY, 1, blockIndex);
	uint32_t blockDataLen = CBDatabaseGetLength((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockIndex, CB_KEY_ARRAY);
	if (blockDataLen == CB_DOESNT_EXIST)
		return NULL;
	blockDataLen -= CB_BLOCK_START;
	// We do want any more than 90 bytes
	if (blockDataLen > 90)
		blockDataLen = 90;
	// Get block data
	CBByteArray * data = CBNewByteArrayOfSize(blockDataLen);
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockIndex, CB_KEY_ARRAY, CBByteArrayGetData(data), blockDataLen, CB_BLOCK_START) != CB_DATABASE_INDEX_FOUND){
		CBLogError("Could not read a block from the database.");
		CBReleaseObject(data);
		return NULL;
	}
	// Trim the data to only include the var int and then add a null byte
	uint8_t varIntSize = CBVarIntDecode(data, 80).size;
	data->length = 81 + varIntSize;
	CBByteArraySetByte(data, data->length - 1, 0);
	// Make and return the block
	CBBlock * block = CBNewBlockFromData(data);
	CBReleaseObject(data);
	return block;
}
bool CBBlockChainStorageGetBlockLocation(void * validator, uint8_t * blockHash, uint8_t * branch, uint32_t * index){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	memcpy(CB_KEY_ARRAY, blockHash, 20);
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockHashIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 5, 0) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a block hash reference from the block chain database.");
		return false;
	}
	*branch = CB_DATA_ARRAY[CB_BLOCK_HASH_REF_BRANCH];
	*index = CBArrayToInt32(CB_DATA_ARRAY, CB_BLOCK_HASH_REF_INDEX);
	return true;
}
uint32_t CBBlockChainStorageGetBlockTime(void * validator, uint8_t branch, uint32_t blockIndex){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	CB_KEY_ARRAY[0] = branch;
	CBInt32ToArray(CB_KEY_ARRAY, 1, blockIndex);
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 4, CB_BLOCK_TIME) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read the time for a block.");
		return 0;
	}
	return CBArrayToInt32(CB_DATA_ARRAY, 0);
}
uint32_t CBBlockChainStorageGetBlockTarget(void * validator, uint8_t branch, uint32_t blockIndex){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	CB_KEY_ARRAY[0] = branch;
	CBInt32ToArray(CB_KEY_ARRAY, 1, blockIndex);
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 4, CB_BLOCK_TARGET) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read the target for a block.");
		return 0;
	}
	return CBArrayToInt32(CB_DATA_ARRAY, 0);
}
bool CBBlockChainStorageIsTransactionWithUnspentOutputs(void * validator, uint8_t * txHash, bool * exists){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	// Place transaction hash into the key
	memcpy(CB_KEY_ARRAY, txHash, 32);
	// See if the transaction exists
	if (CBDatabaseGetLength((CBDatabase *)storageObj, &storageObj->tx, storageObj->txIndex, CB_KEY_ARRAY) == CB_DOESNT_EXIST){
		*exists = false;
		return true;
	}
	// Now see if the transaction has unspent outputs
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->txIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 4, CB_TX_REF_NUM_UNSPENT_OUTPUTS) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read the number of unspent outputs for a transaction.");
		return false;
	}
	*exists = CBArrayToInt32(CB_DATA_ARRAY, 0);
	return true;
}
bool CBBlockChainStorageLoadBasicValidator(void * validator){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->branchIndex, (uint8_t []){CB_MAX_BRANCH_CACHE}, CB_DATA_ARRAY, 4, 0) != CB_DATABASE_INDEX_FOUND){
		CBLogError("There was an error when reading the validator information from storage.");
		return false;
	}
	validatorObj->firstOrphan = CB_DATA_ARRAY[CB_VALIDATION_FIRST_ORPHAN];
	validatorObj->numOrphans = CB_DATA_ARRAY[CB_VALIDATION_NUM_ORPHANS];
	validatorObj->mainBranch = CB_DATA_ARRAY[CB_VALIDATION_MAIN_BRANCH];
	validatorObj->numBranches = CB_DATA_ARRAY[CB_VALIDATION_NUM_BRANCHES];
	return true;
}
void * CBBlockChainStorageLoadBlock(void * validator, uint32_t blockID, uint32_t branch){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	CB_KEY_ARRAY[0] = branch;
	CBInt32ToArray(CB_KEY_ARRAY, 1, blockID);
	uint32_t blockDataLen = CBDatabaseGetLength((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockIndex, CB_KEY_ARRAY);
	if (blockDataLen == CB_DOESNT_EXIST)
		return NULL;
	blockDataLen -= CB_BLOCK_START;
	// Get block data
	CBByteArray * data = CBNewByteArrayOfSize(blockDataLen);
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockIndex, CB_KEY_ARRAY, CBByteArrayGetData(data), blockDataLen, CB_BLOCK_START) != CB_DATABASE_INDEX_FOUND){
		CBLogError("Could not read a block from the database.");
		CBReleaseObject(data);
		return NULL;
	}
	// Make and return the block
	CBBlock * block = CBNewBlockFromData(data);
	CBReleaseObject(data);
	return block;
}
bool CBBlockChainStorageLoadBranch(void * validator, uint8_t branchNum){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	// Get simple information
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->branchIndex, (uint8_t []){branchNum}, CB_DATA_ARRAY, 21, 0) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("There was an error when reading the data for a branch's information.");
		return false;
	}
	validatorObj->branches[branchNum].lastRetargetTime = CBArrayToInt32(CB_DATA_ARRAY, CB_BRANCH_LAST_RETARGET);
	validatorObj->branches[branchNum].lastValidation = CBArrayToInt32(CB_DATA_ARRAY, CB_BRANCH_LAST_VALIDATION);
	validatorObj->branches[branchNum].numBlocks = CBArrayToInt32(CB_DATA_ARRAY, CB_BRANCH_NUM_BLOCKS);
	validatorObj->branches[branchNum].parentBlockIndex = CBArrayToInt32(CB_DATA_ARRAY, CB_BRANCH_PARENT_BLOCK_INDEX);
	validatorObj->branches[branchNum].parentBranch = CB_DATA_ARRAY[CB_BRANCH_PARENT_BRANCH];
	validatorObj->branches[branchNum].startHeight = CBArrayToInt32(CB_DATA_ARRAY, CB_BRANCH_START_HEIGHT);
	validatorObj->branches[branchNum].working = false; // Start off not working on any branch.
	return true;
}
bool CBBlockChainStorageLoadBranchWork(void * validator, uint8_t branchNum){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	// Get work
	uint8_t workLen = CBDatabaseGetLength((CBDatabase *)storageObj, &storageObj->tx, storageObj->branchWorkIndex, (uint8_t []){branchNum});
	if (workLen == CB_DOESNT_EXIST) {
		CBLogError("Attempting to load branch work that does not exist.");
		return false;
	}
	CBBigIntAlloc(&validatorObj->branches[branchNum].work, workLen);
	validatorObj->branches[branchNum].work.length = workLen;
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->branchWorkIndex, (uint8_t []){branchNum}, validatorObj->branches[branchNum].work.data, workLen, 0) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("There was an error when reading the work for a branch.");
		free(validatorObj->branches[branchNum].work.data);
		return false;
	}
	return true;
}
bool CBBlockChainStorageLoadOrphan(void * validator, uint8_t orphanNum){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	uint32_t len = CBDatabaseGetLength((CBDatabase *)storageObj, &storageObj->tx, storageObj->orphanIndex, (uint8_t []){orphanNum});
	if (len == CB_DOESNT_EXIST) {
		CBLogError("Attempting to load an orphan that does not exist.");
		return false;
	}
	CBByteArray * orphanData = CBNewByteArrayOfSize(len);
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->orphanIndex, (uint8_t []){orphanNum}, CBByteArrayGetData(orphanData), len, 0) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("There was an error when reading the data for an orphan.");
		CBReleaseObject(orphanData);
		return false;
	}
	validatorObj->orphans[orphanNum] = CBNewBlockFromData(orphanData);
	CBReleaseObject(orphanData);
	return true;
}
bool CBBlockChainStorageLoadOutputs(void * validator, uint8_t * txHash, uint8_t ** data, uint32_t * dataAllocSize, uint32_t * position){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	memcpy(CB_KEY_ARRAY, txHash, 32);
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->txIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 14, 0) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction reference from the transaction index.");
		return false;
	}
	// Set position of outputs
	*position = CBArrayToInt32(CB_DATA_ARRAY, CB_TX_REF_POSITION_OUPTUTS);
	// Get transaction to find position for output in the block
	// Reallocate transaction data memory if needed.
	if (CBArrayToInt32(CB_DATA_ARRAY, CB_TX_REF_LENGTH_OUTPUTS) > *dataAllocSize) {
		*dataAllocSize = CBArrayToInt32(CB_DATA_ARRAY, CB_TX_REF_LENGTH_OUTPUTS);
		*data = realloc(*data, *dataAllocSize);
	}
	// Read transaction from the block
	CB_KEY_ARRAY[0] = CB_DATA_ARRAY[CB_TX_REF_BRANCH];
	memcpy(CB_KEY_ARRAY + 1, CB_DATA_ARRAY + CB_TX_REF_BLOCK_INDEX, 4);
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockIndex, CB_KEY_ARRAY, *data, CBArrayToInt32(CB_DATA_ARRAY, CB_TX_REF_LENGTH_OUTPUTS), CB_BLOCK_START + *position) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction from the block-chain database.");
		return false;
	}
	return true;
}
void * CBBlockChainStorageLoadUnspentOutput(void * validator, uint8_t * txHash, uint32_t outputIndex, bool * coinbase, uint32_t * outputHeight){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	// First read data for the unspent output key.
	memcpy(CB_KEY_ARRAY, txHash, 32);
	CBInt32ToArray(CB_KEY_ARRAY, 32, outputIndex);
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->unspentOutputIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 8, 0) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Cannot read unspent output information from the block chain database");
		return NULL;
	}
	uint32_t outputPosition = CBArrayToInt32(CB_DATA_ARRAY, CB_UNSPENT_OUTPUT_REF_POSITION);
	uint32_t outputLength = CBArrayToInt32(CB_DATA_ARRAY, CB_UNSPENT_OUTPUT_REF_LENGTH);
	// Now read data for the transaction
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->txIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 14, 0) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Cannot read a transaction reference from the transaction index.");
		return NULL;
	}
	uint8_t outputBranch = CB_DATA_ARRAY[CB_TX_REF_BRANCH];
	uint32_t outputBlockIndex = CBArrayToInt32(CB_DATA_ARRAY, CB_TX_REF_BLOCK_INDEX);
	// Set coinbase
	*coinbase = CB_DATA_ARRAY[CB_TX_REF_IS_COINBASE];
	// Set output height
	*outputHeight = validatorObj->branches[outputBranch].startHeight + outputBlockIndex;
	// Get the output from storage
	CB_KEY_ARRAY[0] = outputBranch;
	CBInt32ToArray(CB_KEY_ARRAY, 1, outputBlockIndex);
	// Get output data
	CBByteArray * outputBytes = CBNewByteArrayOfSize(outputLength);
	if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockIndex, CB_KEY_ARRAY, CBByteArrayGetData(outputBytes), outputLength, CB_BLOCK_START + outputPosition) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read an unspent output");
		CBReleaseObject(outputBytes);
		return NULL;
	}
	// Create output object
	CBTransactionOutput * output = CBNewTransactionOutputFromData(outputBytes);
	CBReleaseObject(outputBytes);
	if (NOT CBTransactionOutputDeserialise(output)) {
		CBLogError("Could not deserialise an unspent output");
		CBReleaseObject(output);
		return NULL;
	}
	return output;
}
bool CBBlockChainStorageMoveBlock(void * validator, uint8_t branch, uint32_t blockIndex, uint8_t newBranch, uint32_t newIndex){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	CB_KEY_ARRAY[0] = branch;
	CBInt32ToArray(CB_KEY_ARRAY, 1, blockIndex);
	uint8_t newKey[5];
	newKey[0] = newBranch;
	CBInt32ToArray(newKey, 1, blockIndex);
	CBDatabaseChangeKey(&storageObj->tx, storageObj->blockIndex, CB_KEY_ARRAY, newKey);
	return true;
}
void CBBlockChainStorageReset(CBDepObject self){
	CBDatabaseClearPending(&((CBBlockChainStorage *)self.ptr)->tx);
}
bool CBBlockChainStorageSaveBasicValidator(void * validator){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	CB_DATA_ARRAY[CB_VALIDATION_FIRST_ORPHAN] = validatorObj->firstOrphan;
	CB_DATA_ARRAY[CB_VALIDATION_NUM_ORPHANS] = validatorObj->numOrphans;
	CB_DATA_ARRAY[CB_VALIDATION_MAIN_BRANCH] = validatorObj->mainBranch;
	CB_DATA_ARRAY[CB_VALIDATION_NUM_BRANCHES] = validatorObj->numBranches;
	CBDatabaseWriteValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->branchIndex, (uint8_t []){CB_MAX_BRANCH_CACHE}, CB_DATA_ARRAY, 4);
	return true;
}
bool CBBlockChainStorageSaveBlock(void * validator, void * block, uint8_t branch, uint32_t blockIndex){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	CBBlock * blockObj = block;
	// Write the block data
	CB_KEY_ARRAY[0] = branch;
	CBInt32ToArray(CB_KEY_ARRAY, 1, blockIndex);
	uint8_t * dataParts[2] = {CBBlockGetHash(blockObj), CBByteArrayGetData(CBGetMessage(blockObj)->bytes)};
	uint32_t dataSizes[2] = {20, CBGetMessage(blockObj)->bytes->length};
	CBDatabaseWriteConcatenatedValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockIndex, CB_KEY_ARRAY, 2, dataParts, dataSizes);
	// Write to the block hash index
	memcpy(CB_KEY_ARRAY, CBBlockGetHash(blockObj), 20);
	CB_DATA_ARRAY[CB_BLOCK_HASH_REF_BRANCH] = branch;
	CBInt32ToArray(CB_DATA_ARRAY, CB_BLOCK_HASH_REF_INDEX, blockIndex);
	CBDatabaseWriteValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockHashIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 5);
	return true;
}
bool CBBlockChainStorageSaveBlockHeader(void * validator, void * block, uint8_t branch, uint32_t blockIndex){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	CBBlock * blockObj = block;
	// Write the block data
	CB_KEY_ARRAY[0] = branch;
	CBInt32ToArray(CB_KEY_ARRAY, 1, blockIndex);
	uint8_t null = 0;
	uint8_t * dataParts[3] = {CBBlockGetHash(blockObj), CBByteArrayGetData(CBGetMessage(blockObj)->bytes), &null};
	uint32_t dataSizes[3] = {20, 80 + CBVarIntDecode(CBGetMessage(blockObj)->bytes, 80).size, 1};
	CBDatabaseWriteConcatenatedValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockIndex, CB_KEY_ARRAY, 3, dataParts, dataSizes);
	// Write to the block hash index
	memcpy(CB_KEY_ARRAY, CBBlockGetHash(blockObj), 20);
	CB_DATA_ARRAY[CB_BLOCK_HASH_REF_BRANCH] = branch;
	CBInt32ToArray(CB_DATA_ARRAY, CB_BLOCK_HASH_REF_INDEX, blockIndex);
	CBDatabaseWriteValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->blockHashIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 5);
	return true;
}
bool CBBlockChainStorageSaveBranch(void * validator, uint8_t branch){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	CB_KEY_ARRAY[0] = branch;
	// Make data
	CBInt32ToArray(CB_DATA_ARRAY, CB_BRANCH_LAST_RETARGET, validatorObj->branches[branch].lastRetargetTime);
	CBInt32ToArray(CB_DATA_ARRAY, CB_BRANCH_LAST_VALIDATION, validatorObj->branches[branch].lastValidation);
	CBInt32ToArray(CB_DATA_ARRAY, CB_BRANCH_NUM_BLOCKS, validatorObj->branches[branch].numBlocks);
	CBInt32ToArray(CB_DATA_ARRAY, CB_BRANCH_PARENT_BLOCK_INDEX, validatorObj->branches[branch].parentBlockIndex);
	CB_DATA_ARRAY[CB_BRANCH_PARENT_BRANCH] = validatorObj->branches[branch].parentBranch;
	CBInt32ToArray(CB_DATA_ARRAY, CB_BRANCH_START_HEIGHT, validatorObj->branches[branch].startHeight);
	// Write data
	CBDatabaseWriteValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->branchIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 21);
	return true;
}
bool CBBlockChainStorageSaveBranchWork(void * validator, uint8_t branch){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	CBDatabaseWriteValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->branchWorkIndex, (uint8_t []){branch}, validatorObj->branches[branch].work.data, validatorObj->branches[branch].work.length);
	return true;
}
bool CBBlockChainStorageSaveOrphan(void * validator, void * block, uint8_t orphanNum){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	CBBlock * blockObj = block;
	CBDatabaseWriteValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->orphanIndex, (uint8_t []){orphanNum}, CBByteArrayGetData(CBGetMessage(blockObj)->bytes), CBGetMessage(blockObj)->bytes->length);
	return true;
}
bool CBBlockChainStorageSaveOrphanHeader(void * validator, void * block, uint8_t orphanNum){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	CBBlock * blockObj = block;
	uint8_t null = 0;
	uint8_t * dataParts[3] = {CBByteArrayGetData(CBGetMessage(blockObj)->bytes), &null};
	uint32_t dataSizes[3] = {80 + CBVarIntDecode(CBGetMessage(blockObj)->bytes, 80).size, 1};
	CBDatabaseWriteConcatenatedValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->orphanIndex, (uint8_t []){orphanNum}, 2, dataParts, dataSizes);
	return true;
}
bool CBBlockChainStorageSaveTransactionRef(void * validator, uint8_t * txHash, uint8_t branch, uint32_t blockIndex, uint32_t outputPos, uint32_t outputsLen, bool coinbase, uint32_t numOutputs){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	memcpy(CB_KEY_ARRAY, txHash, 32);
	if (CBDatabaseGetLength((CBDatabase *)storageObj, &storageObj->tx, storageObj->txIndex, CB_KEY_ARRAY) != CB_DOESNT_EXIST) {
		// We have the transaction already. Thus increase the instance count.
		if (CBDatabaseReadValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->txIndex, CB_KEY_ARRAY, CB_DATA_ARRAY + 4, 4,  CB_TX_REF_NUM_UNSPENT_OUTPUTS) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Could not read a transaction reference from the transaction index.");
			return false;
		}
		// Increase the instance count. We change nothing else as we will use the first instance for all other instances.
		CBInt32ToArray(CB_DATA_ARRAY, 4, CBArrayToInt32(CB_DATA_ARRAY, 4) + 1);
		// Set the number of unspent outputs back to the number of outputs in the transaction
		CBInt32ToArray(CB_DATA_ARRAY,0, numOutputs);
		CBDatabaseWriteValueSubSection((CBDatabase *)storageObj, &storageObj->tx, storageObj->txIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 8,  CB_TX_REF_NUM_UNSPENT_OUTPUTS);
	}else{
		// This transaction has not yet been seen in the block chain.
		CBInt32ToArray(CB_DATA_ARRAY, CB_TX_REF_BLOCK_INDEX, blockIndex);
		CB_DATA_ARRAY[CB_TX_REF_BRANCH] = branch;
		CBInt32ToArray(CB_DATA_ARRAY, CB_TX_REF_POSITION_OUPTUTS, outputPos);
		CBInt32ToArray(CB_DATA_ARRAY, CB_TX_REF_LENGTH_OUTPUTS, outputsLen);
		CB_DATA_ARRAY[CB_TX_REF_IS_COINBASE] = coinbase;
		// We start with an instance count of one
		CBInt32ToArray(CB_DATA_ARRAY, CB_TX_REF_INSTANCE_COUNT, 1);
		// Set the number of unspent outputs back to the number of outputs in the transaction
		CBInt32ToArray(CB_DATA_ARRAY, CB_TX_REF_NUM_UNSPENT_OUTPUTS, numOutputs);
		// Write to the transaction index.
		CBDatabaseWriteValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->txIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 22);
	}
	return true;
}
bool CBBlockChainStorageSaveUnspentOutput(void * validator, uint8_t * txHash, uint32_t outputIndex, uint32_t position, uint32_t length, bool increment){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	memcpy(CB_KEY_ARRAY, txHash, 32);
	CBInt32ToArray(CB_KEY_ARRAY, 32, outputIndex);
	CBInt32ToArray(CB_DATA_ARRAY, CB_UNSPENT_OUTPUT_REF_POSITION, position);
	CBInt32ToArray(CB_DATA_ARRAY, CB_UNSPENT_OUTPUT_REF_LENGTH, length);
	// Add to storage
	CBDatabaseWriteValue((CBDatabase *)storageObj, &storageObj->tx, storageObj->unspentOutputIndex, CB_KEY_ARRAY, CB_DATA_ARRAY, 8);
	if (increment
		// For the transaction, increment the number of unspent outputs
		&& NOT CBBlockChainStorageChangeUnspentOutputsNum(storageObj, txHash, +1)) {
		CBLogError("Could not increment the number of unspent outputs for a transaction.");
		return false;
	}
	return true;
}
bool CBBlockChainStorageUnspentOutputExists(void * validator, uint8_t * txHash, uint32_t outputIndex){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	memcpy(CB_KEY_ARRAY, txHash, 32);
	CBInt32ToArray(CB_KEY_ARRAY, 32, outputIndex);
	return CBDatabaseGetLength((CBDatabase *)storageObj, &storageObj->tx, storageObj->unspentOutputIndex, CB_KEY_ARRAY) != CB_DOESNT_EXIST;
}
