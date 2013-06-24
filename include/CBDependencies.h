//
//  CBDependencies.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 15/05/2012.
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
 @brief File for weak linked functions for dependency injection. All these functions are unimplemented. The functions include the crytography functions which are key for the functioning of bitcoin. Sockets must be non-blocking and asynchronous. The use of the sockets is designed to be compatible with libevent. The random number functions should be cryptographically secure. See the dependecies folder for implementations.
 */

#ifndef CBDEPENDENCIESH
#define CBDEPENDENCIESH

#include <stdint.h>
#include <stdbool.h>
#include "CBConstants.h"

// Use weak linking so these functions can be implemented outside of the library.

// Weak linking for cryptographic functions.

#pragma weak CBSha256
#pragma weak CBRipemd160
#pragma weak CBSha160
#pragma weak CBEcdsaVerify

// Weak linking for networking functions.

#pragma weak CBNewSocket
#pragma weak CBSocketBind
#pragma weak CBSocketConnect
#pragma weak CBSocketListen
#pragma weak CBSocketAccept
#pragma weak CBNewEventLoop
#pragma weak CBSocketCanAcceptEvent
#pragma weak CBSocketDidConnectEvent
#pragma weak CBSocketCanSendEvent
#pragma weak CBSocketCanReceiveEvent
#pragma weak CBSocketAddEvent
#pragma weak CBSocketRemoveEvent
#pragma weak CBSocketFreeEvent
#pragma weak CBSocketSend
#pragma weak CBSocketReceive
#pragma weak CBStartTimer
#pragma weak CBEndTimer
#pragma weak CBCloseSocket
#pragma weak CBExitEventLoop

// Weak linking for random number generation functions.

#pragma weak CBNewSecureRandomGenerator
#pragma weak CBSecureRandomSeed
#pragma weak CBRandomSeed
#pragma weak CBSecureRandomInteger
#pragma weak CBFreeSecureRandomGenerator

// Weak linking for block storage functions

#pragma weak CBNewBlockChainStorage
#pragma weak CBFreeBlockChainStorage
#pragma weak CBBlockChainStorageBlockExists
#pragma weak CBBlockChainStorageCommitData
#pragma weak CBBlockChainStorageDeleteBlock
#pragma weak CBBlockChainStorageDeleteUnspentOutput
#pragma weak CBBlockChainStorageDeleteTransactionRef
#pragma weak CBBlockChainStorageExists
#pragma weak CBBlockChainStorageGetBlockHash
#pragma weak CBBlockChainStorageGetBlockHeader
#pragma weak CBBlockChainStorageGetBlockLocation
#pragma weak CBBlockChainStorageGetBlockTime
#pragma weak CBBlockChainStorageGetBlockTarget
#pragma weak CBBlockChainStorageLoadBasicValidator
#pragma weak CBBlockChainStorageLoadBlock
#pragma weak CBBlockChainStorageLoadBranch
#pragma weak CBBlockChainStorageLoadBranchWork
#pragma weak CBBlockChainStorageLoadOrphan
#pragma weak CBBlockChainStorageLoadOutputs
#pragma weak CBBlockChainStorageLoadUnspentOutput
#pragma weak CBBlockChainStorageMoveBlock
#pragma weak CBBlockChainStorageReset
#pragma weak CBBlockChainStorageSaveBasicValidator
#pragma weak CBBlockChainStorageSaveBlock
#pragma weak CBBlockChainStorageSaveBlockHeader
#pragma weak CBBlockChainStorageSaveBranch
#pragma weak CBBlockChainStorageSaveBranchWork
#pragma weak CBBlockChainStorageSaveOrphan
#pragma weak CBBlockChainStorageSaveOrphanHeader
#pragma weak CBBlockChainStorageSaveTransactionRef
#pragma weak CBBlockChainStorageSaveUnspentOutput
#pragma weak CBBlockChainStorageUnspentOutputExists

// Weak linking for address storage functions

#pragma weak CBNewAddressStorage
#pragma weak CBFreeAddressStorage
#pragma weak CBAddressStorageLoadAddresses
#pragma weak CBAddressStorageSaveAddress

// Weak linking for logging dependencies

#pragma weak CBLogError

// Weak linking for timing dependencies

#pragma weak CBGetMilliseconds

// union for dependency objects

typedef union{
	void * ptr;
	int i;
} CBDepObject;

// CRYPTOGRAPHIC DEPENDENCIES

/**
 @brief SHA-256 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @param output A pointer to hold a 32-byte hash.
 */
void CBSha256(uint8_t * data, uint16_t length, uint8_t * output);
/**
 @brief RIPEMD-160 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @param output A pointer to hold a 20-byte hash.
 */
void CBRipemd160(uint8_t * data, uint16_t length, uint8_t * output);
/**
 @brief SHA-1 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @param output A pointer to hold a 10-byte hash.
 */
void CBSha160(uint8_t * data, uint16_t length, uint8_t * output);
/**
 @brief Verifies an ECDSA signature. This function must stick to the cryptography requirements in OpenSSL version 1.0.0 or any other compatible version. There may be compatibility problems when using libraries or code other than OpenSSL since OpenSSL does not adhere fully to the SEC1 ECDSA standards. This could cause security problems in your code. If in doubt, stick to OpenSSL.
 @param signature BER encoded signature bytes.
 @param sigLen The length of the signature bytes.
 @param hash A 32 byte hash for checking the signature against.
 @param pubKey Public key bytes to check this signature with.
 @param keyLen The length of the public key bytes.
 @returns true if the signature is valid and false if invalid.
 */
bool CBEcdsaVerify(uint8_t * signature, uint8_t sigLen, uint8_t * hash, const uint8_t * pubKey, uint8_t keyLen);

// NETWORKING DEPENDENCIES

// Constants

typedef enum{
	CB_TIMEOUT_CONNECT,
	CB_TIMEOUT_RESPONSE,
	CB_TIMEOUT_NO_DATA,
	CB_TIMEOUT_SEND,
	CB_TIMEOUT_RECEIVE
} CBTimeOutType;

typedef enum{
	CB_SOCKET_OK,
	CB_SOCKET_NO_SUPPORT,
	CB_SOCKET_BAD
} CBSocketReturn;

#define CB_SOCKET_CONNECTION_CLOSE -1
#define CB_SOCKET_FAILURE -2

// Functions

/**
 @brief Creates a new TCP/IP socket. The socket should use a non-blocking mode.
 @param socketID Pointer to CBDepObject for the socket
 @param IPv6 true if the socket is used to connect to the IPv6 network.
 @returns CB_SOCKET_OK if the socket was successfully created, CB_SOCKET_NO_SUPPORT and CB_SOCKET_BAD if the socket could not be created for any other reason.
 */
CBSocketReturn CBNewSocket(CBDepObject * socketID, bool IPv6);
/*
 @brief Binds the host machine and a port number to a new socket.
 @param socketID The socket id to set to the new socket.
 @param IPv6 true if we are binding a socket for the IPv6 network.
 @param port The port to bind to.
 @returns true if the bind was sucessful and false otherwise.
 */
bool CBSocketBind(CBDepObject * socketID, bool IPv6, uint16_t port);
/**
 @brief Begin connecting to an external host with a socket. This should be non-blocking.
 @param socketID The socket id
 @param IP 16 bytes for an IPv6 address to connect to.
 @param IPv6 True if IP address is for the IPv6 network. A IPv6 address can represent addresses for IPv4 too. To avoid the need to detect this, a boolean can be used.
 @param port Port to connect to.
 @returns true if the function was sucessful and false otherwise.
 */
bool CBSocketConnect(CBDepObject socketID, uint8_t * IP, bool IPv6, uint16_t port);
/**
 @brief Begin listening for incomming connections on a bound socket. This should be non-blocking. 
 @param socketID The socket id
 @param maxConnections The maximum incomming connections to allow.
 @returns true if function was sucessful and false otherwise.
 */
bool CBSocketListen(CBDepObject socketID, uint16_t maxConnections);
/**
 @brief Accepts an incomming IPv4 connection on a bound socket. This should be non-blocking.
 @param socketID The socket id
 @param connectionSocketID A socket id for a new socket for the connection.
 @returns true if function was sucessful and false otherwise.
 */
bool CBSocketAccept(CBDepObject socketID, CBDepObject * connectionSocketID);
/**
 @brief Starts a event loop for socket events on a seperate thread. Access to the loop id should be thread safe.
 @param loopID A CBDepObject storing an integer or pointer representation of the new event loop.
 @param onError If the event loop fails during execution of the thread, this function should be called.
 @param onDidTimeout The function to call for timeout event. The second argument is for the peer. The third is for the timeout type. For receiving data, the timeout should be CB_TIMEOUT_RECEIVE. The CBNetworkCommunicator will determine if it should be changed to CB_TIMEOUT_RESPONSE.
 @param communicator A CBNetworkCommunicator to pass to all event functions (first parameter), including "onError" and "onDidTimeout"
 @returns true on success, false on failure.
 */
bool CBNewEventLoop(CBDepObject * loopID, void (*onError)(void *), void (*onDidTimeout)(void *, void *, CBTimeOutType), void * communicator);
/**
 @brief Creates an event where a listening socket is available for accepting a connection. The event should be persistent and not issue timeouts.
 @param eventID The event object to set.
 @param loopID The loop id.
 @param socketID The socket id.
 @param onCanAccept The function to call for the event. Accepts "onEventArg" and the socket ID.
 @returns true on success, false on failure.
 */
bool CBSocketCanAcceptEvent(CBDepObject * eventID, CBDepObject loopID, CBDepObject socketID, void (*onCanAccept)(void *, CBDepObject));
/**
 @brief Sets a function pointer for the event where a socket has connected. The event only needs to fire once on the successful connection or timeout.
 @param eventID The event object to set.
 @param loopID The loop id.
 @param socketID The socket id
 @param onDidConnect The function to call for the event.
 @param peer The peer to send to the "onDidConnect" or "onDidTimeout" function.
 @returns true on success, false on failure.
 */
bool CBSocketDidConnectEvent(CBDepObject * eventID, CBDepObject loopID, CBDepObject socketID, void (*onDidConnect)(void *, void *), void * peer);
/**
 @brief Sets a function pointer for the event where a socket is available for sending data. This should be persistent.
 @param eventID The event object to set.
 @param loopID The loop id.
 @param socketID The socket id
 @param onCanSend The function to call for the event.
 @param peer The peer to send to the "onCanSend" or "onDidTimeout" function.
 @returns true on success, false on failure.
 */
bool CBSocketCanSendEvent(CBDepObject * eventID, CBDepObject loopID, CBDepObject socketID, void (*onCanSend)(void *, void *), void * peer);
/**
 @brief Sets a function pointer for the event where a socket is available for receiving data. This should be persistent.
 @param eventID The event object to set.
 @param loopID The loop id.
 @param socketID The socket id
 @param onCanReceive The function to call for the event.
 @param peer The peer to send to the "onCanReceive" or "onDidTimeout" function.
 @returns true on success, false on failure.
 */
bool CBSocketCanReceiveEvent(CBDepObject * eventID, CBDepObject loopID, CBDepObject socketID, void (*onCanReceive)(void *, void *), void * peer);
/**
 @brief Adds an event to be pending.
 @param eventID The event ID to add.
 @param timeout The time in milliseconds to issue a timeout for the event. 0 for no timeout.
 @returns true if sucessful, false otherwise.
 */
bool CBSocketAddEvent(CBDepObject eventID, uint32_t timeout);
/**
 @brief Removes an event so no more events are made.
 @param eventID The event ID to remove
 @returns true if sucessful, false otherwise.
 */
bool CBSocketRemoveEvent(CBDepObject eventID);
/**
 @brief Makes an event non-pending and frees it.
 @param eventID The event to free.
 */
void CBSocketFreeEvent(CBDepObject eventID);
/**
 @brief Sends data to a socket. This should be non-blocking.
 @param socketID The socket id to send to.
 @param data The data bytes to send.
 @param len The length of the data to send.
 @returns The number of bytes actually sent, and CB_SOCKET_FAILURE on failure that suggests further data cannot be sent.
 */
int32_t CBSocketSend(CBDepObject socketID, uint8_t * data, uint32_t len);
/**
 @brief Receives data from a socket. This should be non-blocking.
 @param socketID The socket id to receive data from.
 @param data The data bytes to write the data to.
 @param len The length of the data.
 @returns The number of bytes actually written into "data", CB_SOCKET_CONNECTION_CLOSE on connection closure, 0 on no bytes received, and CB_SOCKET_FAILURE on failure.
 */
int32_t CBSocketReceive(CBDepObject socketID, uint8_t * data, uint32_t len);
/**
 @brief Calls a callback every "time" seconds, until the timer is ended.
 @param loopID The loop id.
 @param timer The timer sent by reference to be set.
 @param time The number of milliseconds between each call of the callback.
 @param callback The callback function.
 @param arg The callback argument.
 */
bool CBStartTimer(CBDepObject loopID, CBDepObject * timer, uint16_t time, void (*callback)(void *), void * arg);
/**
 @brief Ends a timer.
 @param timer The timer.
 */
void CBEndTimer(CBDepObject timer);
/**
 @brief Closes a socket. The id should be freed, as well as any other data relating to this socket.
 @param socketID The socket id to be closed.
 */
void CBCloseSocket(CBDepObject socketID);
/**
 @brief Exits an event loop and frees all data relating to it.
 @param loopID The loop ID. If zero, do nothing.
 */
void CBExitEventLoop(CBDepObject loopID);

// RANDOM DEPENDENCIES

/**
 @brief Returns an instance of a cryptographically secure random number generator.
 @param The generator as a pointer or integer.
 @returns true on success, false on failure.
 */
bool CBNewSecureRandomGenerator(CBDepObject * gen);
/**
 @brief Seeds the random number generator securely.
 @param gen The generator.
 @retruns true if the random number generator was securely seeded, or false otherwise.
 */
bool CBSecureRandomSeed(CBDepObject gen);
/**
 @brief Seeds the generator from a 64-bit integer.
 @param gen The generator.
 @param seed The 64-bit integer.
 */
void CBRandomSeed(CBDepObject gen, uint64_t seed);
/**
 @brief Generates a 64 bit integer.
 @param gen The generator.
 @returns The random 64-bit integer integer.
 */
uint64_t CBSecureRandomInteger(CBDepObject gen);
/**
 @brief Frees the random number generator.
 @param gen The generator.
 */
void CBFreeSecureRandomGenerator(CBDepObject gen);

// BLOCK CHAIN STORAGE DEPENDENCES

/**
 @brief Returns the object used for block-chain storage.
 @param storage The storage object to set.
 @param dataDir The directory where the data files should be stored.
 @returns true on success or false on failure.
 */
bool CBNewBlockChainStorage(CBDepObject * storage, char * dataDir);
/**
 @brief Frees the block-chain storage object.
 @param iself The block-chain storage object.
 */
void CBFreeBlockChainStorage(CBDepObject iself);
/**
 @brief Determines if the block is already in the storage.
 @param validator A CBValidator object. The storage object can be found within this.
 @param blockHash The 20 bytes of the hash of the block
 @returns true if the block is in the storage or false otherwise.
 */
bool CBBlockChainStorageBlockExists(void * validator, uint8_t * blockHash);
/**
 @brief The data should be written to the disk atomically.
 @param iself The block-chain storage object.
 @returns true on success and false on failure. cbitcoin will reload the storafe contents on failure. Storage systems should use recovery.
 */
bool CBBlockChainStorageCommitData(CBDepObject iself);
/**
 @brief Deletes a block.
 @param validator A CBValidator object. The storage object can be found within this.
 @param branch The branch where the block exists.
 @param blockIndex The index of the block.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageDeleteBlock(void * validator, uint8_t branch, uint32_t blockIndex);
/**
 @brief Deletes an unspent output reference. The ouput should still be in the block. The number of unspent outputs for the transaction should be decremented.
 @param validator A CBValidator object. The storage object can be found within this.
 @param txHash The transaction hash of this output.
 @param outputIndex The index of the output in the transaction.
 @param decrement If true decrement the number of unspent outputs for the transaction.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageDeleteUnspentOutput(void * validator, uint8_t * txHash, uint32_t outputIndex, bool decrement);
/**
 @brief Deletes a transaction reference once all instances have been removed. Decrement the instance count and remove if zero. The transaction should still be in the block. Reset the unspent output count to 0.
 @param validator A CBValidator object. The storage object can be found within this.
 @param txHash The transaction's hash.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageDeleteTransactionRef(void * validator, uint8_t * txHash);
/**
 @brief Determines if there is previous data or if initial data needs to be created.
 @param iself The block-chain storage object.
 @returns true if there is previous block-chain data or false if initial data is needed.
 */
bool CBBlockChainStorageExists(CBDepObject iself);
/**
 @brief Obtains the hash for a block.
 @param validator A CBValidator object. The storage object can be found within this.
 @param branch The branch where the block exists.
 @param blockIndex The index of the block.
 @param hash The hash to set.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageGetBlockHash(void * validator, uint8_t branch, uint32_t blockIndex, uint8_t hash[]);
/**
 @brief Obtains the header for a block, which has not be deserialised.
 @param validator A CBValidator object. The storage object can be found within this.
 @param branch The branch where the block exists.
 @param blockIndex The index of the block.
 @returns The block header on sucess or NULL on failure.
 */
void * CBBlockChainStorageGetBlockHeader(void * validator, uint8_t branch, uint32_t blockIndex);
/**
 @brief Gets the location of a block.
 @param validator A CBValidator object. The storage object can be found within this.
 @param blockHash The 20 bytes of the hash of the block
 @param branch Will be set to the branch of the block.
 @param index Will be set to the index of the block.
 @returns true on success or false otherwise.
 */
bool CBBlockChainStorageGetBlockLocation(void * validator, uint8_t * blockHash, uint8_t * branch, uint32_t * index);
/**
 @brief Obtains the timestamp for a block.
 @param validator A CBValidator object. The storage object can be found within this.
 @param branch The branch where the block exists.
 @param blockIndex The index of the block.
 @returns The time on sucess or 0 on failure.
 */
uint32_t CBBlockChainStorageGetBlockTime(void * validator, uint8_t branch, uint32_t blockIndex);
/**
 @brief Obtains the target for a block.
 @param validator A CBValidator object. The storage object can be found within this.
 @param branch The branch where the block exists.
 @param blockIndex The index of the block.
 @returns The target on sucess or 0 on failure.
 */
uint32_t CBBlockChainStorageGetBlockTarget(void * validator, uint8_t branch, uint32_t blockIndex);
/**
 @brief Determines if a transaction exists in the index, which has more than zero unspent outputs.
 @param validator A CBValidator object. The storage object can be found within this.
 @param txHash The hash of the transaction to search for.
 @param exists To be set to true if the transaction exists and has unspent outputs, else set to false.
 @param true on success or false on failure.
 */
bool CBBlockChainStorageIsTransactionWithUnspentOutputs(void * validator, uint8_t * txHash, bool * exists);
/**
 @brief Loads the basic validator information, not any branches or orphans.
 @param validator A CBValidator object. The storage object can be found within this.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageLoadBasicValidator(void * validator);
/**
 @brief Loads a block from storage.
 @param validator The CBValidator object.
 @param blockID The index of the block int he branch.
 @param branch The branch the block belongs to.
 @returns A new CBBlock object with serailised block data which has not been deserialised or NULL on failure.
 */
void * CBBlockChainStorageLoadBlock(void * validator, uint32_t blockID, uint32_t branch);
/**
 @brief Loads a branch
 @param validator A CBValidator object. The storage object can be found within this.
 @param branchNum The number of the branch to load.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageLoadBranch(void * validator, uint8_t branchNum);
/**
 @brief Loads a branch's work
 @param validator A CBValidator object. The storage object can be found within this.
 @param branchNum The number of the branch to load the work for.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageLoadBranchWork(void * validator, uint8_t branchNum);
/**
 @brief Loads an orphan.
 @param validator A CBValidator object. The storage object can be found within this.
 @param orphanNum The number of the orphan to load.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageLoadOrphan(void * validator, uint8_t orphanNum);
/**
 @brief Obtains the outputs for a transaction
 @param validator A CBValidator object. The storage object can be found within this.
 @param txHash The transaction hash to load the output data for.
 @param data A pointer to the buffer to set to the data.
 @param dataAllocSize If the size of the outputs is beyond this number, data will be reallocated to the size of the outputs and the number will be increased to the size of the outputs.
 @param position The position of the outputs in the block to be set.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageLoadOutputs(void * validator, uint8_t * txHash, uint8_t ** data, uint32_t * dataAllocSize, uint32_t * position);
/**
 @brief Obtains an unspent output.
 @param validator A CBValidator object. The storage object can be found within this.
 @param txHash The transaction hash of this output.
 @param outputIndex The index of the output in the transaction.
 @param coinbase Will be set to true if the output exists in a coinbase transaction or false otherwise.
 @param outputHeight The height of the block the output exists in.
 @returns The output as a CBTransactionOutput object on sucess or NULL on failure.
 */
void * CBBlockChainStorageLoadUnspentOutput(void * validator, uint8_t * txHash, uint32_t outputIndex, bool * coinbase, uint32_t * outputHeight);
/**
 @brief Moves a block to a new location.
 @param validator A CBValidator object. The storage object can be found within this.
 @param branch The branch where the block exists.
 @param blockIndex The index of the block.
 @param newBranch The branch where the block will be moved into.
 @param newIndex The index the block will take in the new branch.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageMoveBlock(void * validator, uint8_t branch, uint32_t blockIndex, uint8_t newBranch, uint32_t newIndex);
/**
 @brief Removes all of the pending operations.
 @param iself The storage object.
 */
void CBBlockChainStorageReset(CBDepObject iself);
/**
 @brief Saves the basic validator information, not any branches or orphans.
 @param validator A CBValidator object. The storage object can be found within this.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageSaveBasicValidator(void * validator);
/**
 @brief Saves a block.
 @param validator A CBValidator object. The storage object can be found within this.
 @param block The block to save.
 @param branch The branch of this block
 @param blockIndex The index of this block.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageSaveBlock(void * validator, void * block, uint8_t branch, uint32_t blockIndex);
/**
 @brief Saves a block, with it's header only.
 @param validator A CBValidator object. The storage object can be found within this.
 @param block The block to save.
 @param branch The branch of this block
 @param blockIndex The index of this block.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageSaveBlockHeader(void * validator, void * block, uint8_t branch, uint32_t blockIndex);
/**
 @brief Saves the branch information without commiting to disk.
 @param validator A CBValidator object. The storage object can be found within this.
 @param branch The branch number to save.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageSaveBranch(void * validator, uint8_t branch);
/**
 @brief Saves the work for a branch without commiting to disk.
 @param validator A CBValidator object. The storage object can be found within this.
 @param branch The branch number with the work to save.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageSaveBranchWork(void * validator, uint8_t branch);
/**
 @brief Saves a block as an orphan
 @param validator A CBValidator object. The storage object can be found within this.
 @param block The block to save.
 @param orphanNum The number of the orphan.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageSaveOrphan(void * validator, void * block, uint8_t orphanNum);
/**
 @brief Saves a block as an orphan, with only the header.
 @param validator A CBValidator object. The storage object can be found within this.
 @param block The block to save.
 @param orphanNum The number of the orphan.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageSaveOrphanHeader(void * validator, void * block, uint8_t orphanNum);
/**
 @brief Saves a transaction reference. If the transaction exists in the block chain already, increment a counter. Else add a new reference.
 @param validator A CBValidator object. The storage object can be found within this.
 @param txHash The transaction's hash.
 @param branch The branch where the transaction exists.
 @param blockIndex The index of the block which contains the transaction.
 @param outputPos The position of the outputs in the block for this transaction.
 @param outputsLen The length of the outputs in this transaction.
 @param coinbase If true the transaction is a coinbase, else it is not.
 @param numOutputs The number of outputs for this transaction.
 @returns true if successful or false otherwise.
 */
bool CBBlockChainStorageSaveTransactionRef(void * validator, uint8_t * txHash, uint8_t branch, uint32_t blockIndex, uint32_t outputPos, uint32_t outputsLen, bool coinbase, uint32_t numOutputs);
/**
 @brief Saves an output as unspent.
 @param validator A CBValidator object. The storage object can be found within this.
 @param txHash The transaction hash of this output.
 @param outputIndex The index of the output in the transaction.
 @param position The position of the unspent output in the block.
 @param length The length of the unspent output in the block.
 @param increment If true, increment the number of unspent outputs for the transaction.
 @returns true if successful or false otherwise.
 */
bool CBBlockChainStorageSaveUnspentOutput(void * validator, uint8_t * txHash, uint32_t outputIndex, uint32_t position, uint32_t length, bool increment);
/**
 @brief Determines if an unspent output exists.
 @param validator A CBValidator object. The storage object can be found within this.
 @param txHash The transaction hash of this output.
 @param outputIndex The index of the output in the transaction.
 @returns true if exists or false if it doesn't.
 */
bool CBBlockChainStorageUnspentOutputExists(void * validator, uint8_t * txHash, uint32_t outputIndex);

// ADDRESS STORAGE DEPENDENCES

/**
 @brief Creates a new address storage object.
 @param storage The object to set.
 @param dataDir The directory where the data files should be stored.
 @returns true on success and false on failure.
 */
bool CBNewAddressStorage(CBDepObject * storage, char * dataDir);
/**
 @brief Frees the address storage object.
 @param iself The address storage object.
 */
void CBFreeAddressStorage(CBDepObject iself);
/**
 @brief Removes an address from storage.
 @param iself The address storage object.
 @param address The address object.
 @returns true on success or false on failure.
 */
bool CBAddressStorageDeleteAddress(CBDepObject iself, void * address);
/**
 @brief Obtains the number of addresses in storage.
 @param iself The address storage object.
 @returns The number of addresses in storage
 */
uint64_t CBAddressStorageGetNumberOfAddresses(CBDepObject iself);
/**
 @brief Loads all of the addresses from storage into an address manager.
 @param iself The address storage object.
 @param addrMan A CBNetworkAddressManager object.
 @returns true on success or false on failure.
 */
bool CBAddressStorageLoadAddresses(CBDepObject iself, void * addrMan);
/**
 @brief Saves an address to storage. If the number of addresses is at "maxAddresses" remove an address to make room.
 @param iself The address storage object.
 @param address The CBNetworkAddress object.
 @returns true on success or false on failure.
 */
bool CBAddressStorageSaveAddress(CBDepObject iself, void * address);

// ACCOUNTER DEPENDENCIES

// Cosntants

#define CB_TX_UNCONFIRMED 0
#define CB_NO_BRANCH UINT8_MAX

typedef enum{
	CB_GET_TX_OK,
	CB_GET_TX_ERROR,
	CB_GET_TX_NONE
} CBGetTxResult;

// Structures

/**
 @brief Details for a transaction on a branch for an account.
 */
typedef struct{
	uint8_t txHash[32];
	uint8_t addrHash[20];
	int64_t amount;
	uint64_t timestamp;
} CBTransactionDetails;

// Functions

/**
 @brief Creates a new account storage object.
 @param accounter The object to set.
 @param dataDir The directory where the data files should be stored.
 @returns true on success and false on failure.
 */
bool CBNewAccounter(CBDepObject * accounter, char * dataDir);
/**
 @brief Frees the account storage object.
 @param iself The account storage object.
 */
void CBFreeAccounter(CBDepObject self);
/**
 @brief Commits data to the disk.
 @param self The accounter object.
 @returns true on success and false on failure.
 */
bool CBAccounterCommit(CBDepObject self);
/**
 @brief Adds a watched output hash to an account.
 @param self The accounter object. 
 @param hash The hash as created by CBTransactionOuputGetHash.
 @param accountID The ID of the account to add the hash for.
 @returns true on success and false on failure.
 */
bool CBAccounterAddWatchedOutputToAccount(CBDepObject self, uint8_t * hash, uint64_t accountID);
/**
 @brief Modifies the information of a transaction which has been added to a branch.
 @param self The accounter object. 
 @param tx The transaction object.
 @param blockHeight The block height at which the transaction is found.
 @param branch The branch in which the transaction is found.
 @returns true on success and false on failure.
 */
bool CBAccounterBranchlessTransactionToBranch(CBDepObject self, void * tx, uint32_t blockHeight, uint8_t branch);
/**
 @brief Commits the accounter information.
 @param self The accounter object.
 @returns true on success and false on failure.
 */
bool CBAccounterCommit(CBDepObject self);
/**
 @brief Removes information for a branch.
 @param self The accounter object.
 @param branch The branch that is removed.
 @returns true on success and false on failure.
 */
bool CBAccounterDeleteBranch(CBDepObject self, uint8_t branch);
/**
 @brief Processes a found transaction on a branch for all of the accounts
 @param self The accounter object.
 @param tx The found transaction.
 @param blockHeight The height the transaction was found.
 @param The timestamp for the transaction.
 @param branch The branch the transaction was found on.
 @returns true on success and false on failure.
 */
bool CBAccounterFoundTransaction(CBDepObject self, void * tx, uint32_t blockHeight, uint32_t time, uint8_t branch);
/**
 @brief Gets the first transaction between times, equal or greater than the txIDCursor. The txIDCursor will be moved past a found transaction's ID.
 @param self The accounter object.
 @param branch The branch to find the first transaction for.
 @param accountID The ID of the account to find the transaction for.
 @param timeMin The minimum time of the transaction.
 @param timeMax The maximum time of the transaction.
 @param txIDCursor A pointer to the transaction ID cursor to find thw next transaction.
 @param details A pointer to a CBTransactionDetails object to be set.
 @returns @see CBGetTxResult.
 */
CBGetTxResult CBAccounterGetFirstTransactionBetween(CBDepObject self, uint8_t branch, uint64_t accountID, uint64_t timeMin, uint64_t timeMax, uint64_t * txIDCursor, CBTransactionDetails * details);
/**
 @brief Processes a transaction being lost for all of the accounts.
 @param self The accounter object.
 @param tx The transaction being lost.
 @returns true on success and false on failure.
 */
bool CBAccounterLostBranchlessTransaction(CBDepObject self, void * tx);
/**
 @brief Gets the object for a new account.
 @param self The accounter object.
 @returns the object of the account of 0 on error.
 */
uint64_t CBAccounterNewAccount(CBDepObject self);
/**
 @brief Processes a new branch, inheriting a parent branch for all of the accounts.
 @param self The accounter object.
 @param newBranch The index of the new branch.
 @param inherit The index of the parent branch.
 @returns true on success and false on failure. 
 */
bool CBAccounterNewBranch(CBDepObject self, uint8_t newBranch, uint8_t inherit);

// LOGGING DEPENDENCIES

/**
 @brief Logs an error.
 @param error The error message followed by arguments displayed by the printf() format.
 */
void CBLogError(char * error, ...);

// TIME DEPENDENCIES

/**
 @brief Millisecond precision time function
 @returns A value which when called twice will reflect the milliseconds passed between each call.
 */
uint64_t CBGetMilliseconds(void);

#endif
