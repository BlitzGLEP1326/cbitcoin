//
//  CBValidator.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 14/09/2012.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

/**
 @file
 @brief Validates blocks, finding the main chain.
 */

#ifndef CBVALIDATORH
#define CBVALIDATORH

#include "CBConstants.h"
#include "CBBlock.h"
#include "CBBigInt.h"
#include "CBValidationFunctions.h"
#include "CBAssociativeArray.h"
#include "CBChainDescriptor.h"
#include <string.h>

// Constants

typedef enum{
	CB_VALIDATOR_DISABLE_POW_CHECK = 1, /**< Does not verify the proof of work during validation. Used for testing. */
	CB_VALIDATOR_HEADERS_ONLY = 2,/** Only validates and stores the headers of blocks */
}CBValidatorFlags;

/**
 @brief The status of a processed block.
 */
typedef enum{
	CB_BLOCK_STATUS_MAIN, /**< The block has extended the main branch and that is it. */
	CB_BLOCK_STATUS_REORG, /**< The block has extended the main branch and has caused a reorganisation of the block chain. */
	CB_BLOCK_STATUS_MAIN_WITH_ORPHANS, /**< The block has extended the main branch and one or more orphans were also added */
	CB_BLOCK_STATUS_SIDE, /**< The block has extended a branch which is not the main branch. */
	CB_BLOCK_STATUS_ORPHAN, /**< The block is an orphan */
	CB_BLOCK_STATUS_BAD, /**< The block did not pass validation. */
	CB_BLOCK_STATUS_BAD_TIME, /**< The block has a bad time */
	CB_BLOCK_STATUS_DUPLICATE, /**< The block has already been received. */
	CB_BLOCK_STATUS_ERROR, /**< There was an error while processing a block */
	CB_BLOCK_STATUS_CONTINUE, /**< Continue with the validation. Only returned by CBValidatorBasicBlockValidation */
	CB_BLOCK_STATUS_NO_NEW, /**< Cannot remove a branch for a new one. */
} CBBlockProcessStatus;

/**
 @brief The return type for CBValidatorCompleteBlockValidation
 */
typedef enum{
	CB_BLOCK_VALIDATION_OK, /**< The validation passed with no problems. */
	CB_BLOCK_VALIDATION_BAD, /**< The block failed valdiation. */
	CB_BLOCK_VALIDATION_ERR, /**< There was an error during the validation processing. */
} CBBlockValidationResult;

#define CB_MAX_ORPHAN_CACHE 20
#define CB_MAX_BRANCH_CACHE 5
#define CB_NO_VALIDATION 0xFFFFFFFF
#define CB_COINBASE_MATURITY 100 // Number of confirming blocks before a block reward can be spent.
#define CB_MAX_SIG_OPS 20000 // Maximum signature operations in a block.
#define CB_BLOCK_ALLOWED_TIME_DRIFT 7200 // 2 Hours from network time

/**
 @brief Describes a point on a chain.
 */
typedef struct{
	uint8_t branch; /**< The branch in the chain */
	uint32_t blockIndex; /**< A block in the branch */
} CBChainPoint;

/**
 @brief Describes the path of a block chain.
 */
typedef struct{
	CBChainPoint points[CB_MAX_BRANCH_CACHE]; /**< A list of last blocks in each branch going back to the genesis branch. */
	uint8_t numBranches; /**< The number of branches in this path. */
} CBChainPath;

/**
 @brief Describes a point on a chain path.
 */
typedef struct{
	uint8_t chainPathIndex; /**< The index of the chain path. */
	uint32_t blockIndex; /**< The block index for the branch this is describing. */
} CBChainPathPoint;

typedef struct{
	CBChainPath newChain; /**< The new main chain path */
	CBChainPathPoint start; /**< The start of the reorganisation on the new chain */
} CBReorgData;

typedef struct{
	uint8_t numOrphansAdded; /**< The number of orphans added. */
	CBBlock * orphans[CB_MAX_ORPHAN_CACHE]; /**< Pointers to the orphan blocks */
} CBOrphansData;

union CBBlockProcessResultData{
	CBReorgData reorgData; /**< Data for when there was a reorganisation. (CB_BLOCK_STATUS_REORG) */
	uint8_t sideBranch; /**< Data for when a block was added to the side branch. It is the index of the branch (CB_BLOCK_STATUS_SIDE). */
	CBOrphansData orphansAdded; /**< The orphans added (CB_BLOCK_STATUS_MAIN_WITH_ORPHANS). */
};

/**
 @brief The return type for CBValidatorProcessBlock
 */
typedef struct{
	CBBlockProcessStatus status; /**< The status of the processed block */
	union CBBlockProcessResultData data; /**< Any data that may be associated with the result */
} CBBlockProcessResult;

/**
 @brief Represents a block branch.
 */
typedef struct{
	uint32_t numBlocks; /**< The number of blocks in the branch */
	uint32_t lastRetargetTime; /**< The block timestamp at the last retarget. */
	uint8_t parentBranch; /**< The branch this branch is connected to. */
	uint32_t parentBlockIndex; /**< The block index in the parent branch which this branch is connected to */
	uint32_t startHeight; /**< The starting height where this branch begins */
	uint32_t lastValidation; /**< The index of the last block in this branch that has been fully validated. */
	CBBigInt work; /**< The total work for this branch. The branch with the highest work is the winner! */
	uint16_t working; /**< The number of peers we are receiving this branch from. */
} CBBlockBranch;

/**
 @brief Structure for CBValidator objects. @see CBValidator.h
 */
typedef struct{
	CBObject base;
	uint8_t numOrphans; /**< The number of orhpans */
	CBBlock * orphans[CB_MAX_ORPHAN_CACHE]; /**< The ophan block references */
	uint8_t firstOrphan; /**< The orphan added first or rather the front of the orhpan queue (for overwriting) */
	uint8_t mainBranch; /**< The index for the main branch */
	uint8_t numBranches; /**< The number of block-chain branches. Cannot exceed CB_MAX_BRANCH_CACHE */
	CBBlockBranch branches[CB_MAX_BRANCH_CACHE]; /**< The block-chain branches. */
	uint64_t storage; /**< The storage component object */
	CBValidatorFlags flags; /**< Flags for validation options */
} CBValidator;

/**
 @brief Creates a new CBValidator object.
 @param storage The block-chain storage component.
 @param flags The flags used for validating this block.
 @returns A new CBValidator object.
 */

CBValidator * CBNewValidator(uint64_t storage, CBValidatorFlags flags);

/**
 @brief Gets a CBValidator from another object. Use this to avoid casts.
 @param self The object to obtain the CBValidator from.
 @returns The CBValidator object.
 */
CBValidator * CBGetValidator(void * self);

/**
 @brief Initialises a CBValidator object.
 @param self The CBValidator object to initialise.
 @param storage The block-chain storage component.
 @param flags The flags used for validating this block.
 @returns true on success, false on failure.
 */
bool CBInitValidator(CBValidator * self, uint64_t storage, CBValidatorFlags flags);

/**
 @brief Release a free all of the objects stored by the CBValidator object.
 @param self The CBValidator object to destroy.
 */
void CBDestroyValidator(void * vself);
/**
 @brief Frees a CBValidator object and also calls CBDestroyValidator.
 @param self The CBValidator object to free.
 */
void CBFreeValidator(void * vself);

// Functions

/**
 @brief Adds a block to a branch.
 @param self The CBValidator object.
 @param branch The index of the branch to add the block to.
 @param block The block to add.
 @param work The new branch work. This is not the block work but the total work upto this block. This is taken by the function and the old work is freed.
 @returns true on success and false on failure.
 */
bool CBValidatorAddBlockToBranch(CBValidator * self, uint8_t branch, CBBlock * block, CBBigInt work);
/**
 @brief Adds a block to the orphans.
 @param self The CBValidator object.
 @param block The block to add.
 @returns true on success and false on error.
 */
bool CBValidatorAddBlockToOrphans(CBValidator * self, CBBlock * block);
/**
 @brief Does basic validation on a block 
 @param self The CBValidator object.
 @param block The block to valdiate.
 @param networkTime The network time.
 @returns The block status.
 */
CBBlockProcessStatus CBValidatorBasicBlockValidation(CBValidator * self, CBBlock * block, uint64_t networkTime);
/**
 @brief Completes the validation for a block during main branch extention or reorganisation.
 @param self The CBValidator object.
 @param branch The branch being validated.
 @param block The block to complete validation for.
 @param height The height of the block.
 @returns CB_BLOCK_VALIDATION_OK if the block passed validation, CB_BLOCK_VALIDATION_BAD if the block failed validation and CB_BLOCK_VALIDATION_ERR on an error.
 */
CBBlockValidationResult CBValidatorCompleteBlockValidation(CBValidator * self, uint8_t branch, CBBlock * block, uint32_t height);
/**
 @brief Frees the orphans in a CBBlockProcessResult.
 @param res The CBBlockProcessResult with orphans to free.
 */
void CBValidatorFreeBlockProcessResultOrphans(CBBlockProcessResult * res);
/**
 @brief Gets the block height of the alst block in the main chain.
 @param self The CBValidator
 @returns The block height.
 */
uint32_t CBValidatorGetBlockHeight(CBValidator * self);
/**
 @brief Gets a chain descriptor object for the main chain.
 @param self The CBValidator
 @returns The chain descriptor or NULL on failure.
 */
CBChainDescriptor * CBValidatorGetChainDescriptor(CBValidator * self);
/**
 @brief Determines the point on chain1 where chain2 intersects (The fork point or where the second chain ends).
 @param chain1 The first chain to get the path point for.
 @param chain2 The second chain to look for an intersection with the first.
 @returns The point of intersection on the first chain.
 */
CBChainPathPoint CBValidatorGetChainIntersection(CBChainPath * chain1, CBChainPath * chain2);
/**
 @brief Gets the chain path for a branch going back down to the genesis branch.
 @param self The CBValidator object.
 @param branch The branch to start at.
 @param blockIndex The block index to start at.
 @returns The chain path for the branch and block index going back to the genesis branch.
 */
CBChainPath CBValidatorGetChainPath(CBValidator * self, uint8_t branch, uint32_t blockIndex);
/**
 @brief Gets the chain path for the main chain.
 @param self The CBValidator object.
 @returns The chain path for the main chain.
 */
CBChainPath CBValidatorGetMainChainPath(CBValidator * self);
/**
 @brief Gets the mimimum time minus one allowed for a new block onto a branch.
 @param self The CBValidator object.
 @param branch The id of the branch.
 @param prevIndex The index of the last block to determine the minimum time minus one for when adding onto this block.
 @returns The time on success, or 0 on failure.
 */
uint32_t CBValidatorGetMedianTime(CBValidator * self, uint8_t branch, uint32_t prevIndex);
/**
 @brief Validates a transaction input.
 @param self The CBValidator object.
 @param branch The branch being validated.
 @param block The block begin validated.
 @param blockHeight The height of the block being validated
 @param transactionIndex The index of the transaction to validate.
 @param inputIndex The index of the input to validate.
 @param value Pointer to the total value of the transaction. This will be incremented by this function with the input value.
 @param sigOps Pointer to the total number of signature operations. This is increased by the signature operations for the input and verified to be less that the maximum allowed signature operations.
 @returns CB_BLOCK_VALIDATION_OK if the transaction passed validation, CB_BLOCK_VALIDATION_BAD if the transaction failed validation and CB_BLOCK_VALIDATION_ERR on an error.
 */
CBBlockValidationResult CBValidatorInputValidation(CBValidator * self, uint8_t branch, CBBlock * block, uint32_t blockHeight, uint32_t transactionIndex, uint32_t inputIndex, uint64_t * value, uint32_t * sigOps);
/**
 @brief Processes a block. Block headers are validated, ensuring the integrity of the transaction data is OK, checking the block's proof of work and calculating the total branch work to the genesis block. If the block extends the main branch complete validation is done. If the block extends a branch to become the new main branch because it has the most work, a re-organisation of the block-chain is done.
 @param self The CBValidator object.
 @param block The block to process.
 @param networkTime The network time.
 @return The status of the block.
 */
CBBlockProcessResult CBValidatorProcessBlock(CBValidator * self, CBBlock * block, uint64_t networkTime);
/**
 @brief Processes a block into a branch. This is used once basic validation is done on a block and it is determined what branch it needs to go into and when this branch is ready to receive the block.
 @param self The CBValidator object.
 @param block The block to process.
 @param networkTime The network time.
 @param branch The branch to add to.
 @param prevBranch The branch of the previous block.
 @param prevBlockIndex The index of the previous block.
 @param prevBlockTarget The target of the previous block.
 @param txHashes The transaction hashes for the block.
 @param result The result to set.
 */
void CBValidatorProcessIntoBranch(CBValidator * self, CBBlock * block, uint64_t networkTime, uint8_t branch, uint8_t prevBranch, uint32_t prevBlockIndex, uint32_t prevBlockTarget, CBBlockProcessResult * result);
/**
 @brief Saves the last validated blocks from startBranch to endBranch
 @param self The CBValidator object.
 @param startBranch The branch with the earliest starting block.
 @param endBranch The endBranch that can go down to the startBranch.
 @returns true if the function executed successfully or false on an error.
 */
bool CBValidatorSaveLastValidatedBlocks(CBValidator * self, uint8_t branches);
/**
 @brief Updates the unspent outputs and transaction index for a branch, for removing a block's transaction information.
 @param self The CBValidator object.
 @param block The block with the transaction data to search for changing unspent outputs.
 @param branch The branch the block is for.
 @param blockIndex The block index in the branch.
 @returns true on successful execution or false on error.
 */
bool CBValidatorUpdateUnspentOutputsBackward(CBValidator * self, CBBlock * block, uint8_t branch, uint32_t blockIndex);
/**
 @brief Updates the unspent outputs and transaction index for a branch, for adding a block's transaction information.
 @param self The CBValidator object.
 @param block The block with the transaction data to search for changing unspent outputs.
 @param branch The branch the block is for.
 @param blockIndex The block index in the branch.
 @returns true on successful execution or false on error.
 */
bool CBValidatorUpdateUnspentOutputsForward(CBValidator * self, CBBlock * block, uint8_t branch, uint32_t blockIndex);
/**
 @brief Updates the unspent outputs and transaction index for a branch in reverse and loads a block to do this.
 @param self The CBValidator object.
 @param branch The branch the block is for.
 @param blockIndex The block index in the branch.
 @param forward If true the indices will be updated when adding blocks, else it will be updated removing blocks for re-organisation.
 @returns true on successful execution or false on error.
 */
bool CBValidatorUpdateUnspentOutputsAndLoad(CBValidator * self, uint8_t branch, uint32_t blockIndex, bool forward);

#endif
