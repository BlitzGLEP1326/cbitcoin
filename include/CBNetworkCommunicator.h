//
//  CBNetworkCommunicator.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 13/07/2012.
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
 @brief Used for communicating to other peers. The network communicator can send and receive bitcoin messages and uses function pointers for message handlers. The timeouts are in milliseconds. It is important to understant that a CBNetworkCommunicator does not guarentee thread safety for everything. Thread safety is only given to the "peers" list. This means it is completely okay to add and remove peers from multiple threads. Two threads may try to access the list at once such as if the CBNetworkCommunicator receives a socket timeout event and tries to remove an peer at the same time as a thread made by a program using cbitcoin tries to add a new peer. When using a CBNetworkCommunicator, threading and networking dependencies need to be satisfied, @see CBDependencies.h Inherits CBObject
*/

#ifndef CBNETWORKCOMMUNICATORH
#define CBNETWORKCOMMUNICATORH

//  Includes

#include "CBPeer.h"
#include "CBDependencies.h"
#include "CBNetworkAddressManager.h"
#include "CBNetworkAddressBroadcast.h"
#include "CBInventoryBroadcast.h"
#include "CBGetBlocks.h"
#include "CBTransaction.h"
#include "CBBlockHeaders.h"
#include "CBPingPong.h"
#include "CBAlert.h"
#include <assert.h>
#include <stdio.h>

// Constants and Macros

#define CBGetNetworkCommunicator(x) ((CBNetworkCommunicator *)x)

typedef enum{
	CB_CONNECT_OK, /**< The connection is OK. */
	CB_CONNECT_NO_SUPPORT, /**< There is no support for this type of connection. */
	CB_CONNECT_ERROR, /**< There was an error with connecting. */
	CB_CONNECT_FAILED, /**< The connection failed. */
} CBConnectReturn;

/*
 @brief Used for CBNetworkCommunicator objects. These flags alter the behaviour of a CBNetworkCommunicator.
 */
typedef enum{
	CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE = 1, /**< Automatically share version and verack messages with new connections. */
	CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY = 2, /**< Automatically discover peers and connect upto the maximum allowed connections using the supplied CBVersion. This involves the exchange of version messages and addresses. */
	CB_NETWORK_COMMUNICATOR_AUTO_PING = 4, /**< Send ping messages every "heartBeat" automatically. If the protocol version in the CBVersion message is 60000 or over, cbitcoin will use the new ping/pong specification. @see PingPong.h */
}CBNetworkCommunicatorFlags;

/*
 @brief The action for a CBNetworkCommunicator to complete after the onMessageReceived handler returns.
 */
typedef enum{
	CB_MESSAGE_ACTION_CONTINUE, /**< Continue as normal */
	CB_MESSAGE_ACTION_DISCONNECT, /**< Disconnect the peer */
	CB_MESSAGE_ACTION_STOP, /**< Stop the CBNetworkCommunicator */
	CB_MESSAGE_ACTION_RETURN /**< Return from the message handler with no further action. */
} CBOnMessageReceivedAction;

/*
 @brief The offsets for the message header data.
 */
typedef enum{
	CB_MESSAGE_HEADER_NETWORK_ID = 0, /**< The network identidier bytes */
	CB_MESSAGE_HEADER_TYPE = 4, /**< The 12 character string for the message type */
	CB_MESSAGE_HEADER_LENGTH = 16, /**< The length of the message */
	CB_MESSAGE_HEADER_CHECKSUM = 20, /**< The checksum of the message */
} CBMessageHeaderOffsets;

/**
 @brief Structure for CBNetworkCommunicator objects. @see CBNetworkCommunicator.h
*/
typedef struct {
	CBObject base; /**< CBObject base structure */
	uint32_t networkID; /**< The 4 byte id for sending and receiving messages for a given network. */
	CBNetworkCommunicatorFlags flags; /**< Flags for the operation of the CBNetworkCommunicator. */
	int32_t version; /**< Used for automatic handshaking. This version will be advertised to peers. */
	CBVersionServices services; /**< Used for automatic handshaking. These services will be advertised */
	CBByteArray * userAgent; /**< Used for automatic handshaking. This user agent will be advertised. */
	uint32_t blockHeight; /** Set to the current block height for advertising to peers during the automated handshake. */
	CBNetworkAddress * ourIPv4; /**< IPv4 network address for us. */
	CBNetworkAddress * ourIPv6; /**< IPv6 network address for us. */
	uint32_t attemptingOrWorkingConnections; /**< All connections being attempted or sucessful */
	uint32_t maxConnections; /**< Maximum number of peers allowed to connect to. */
	uint32_t numIncommingConnections; /**< Number of incomming connections made */
	uint32_t maxIncommingConnections; /**< Maximum number of incomming connections. */
	CBNetworkAddressManager * addresses; /**< All addresses both connected and unconnected */
	uint64_t maxAddresses; /**< The maximum number of addresses to store */
	uint32_t heartBeat; /**< If the CB_NETWORK_COMMUNICATOR_AUTO_PING flag is set, the CBNetworkCommunicator will send a "ping" message to all peers after this interval. The default is 1800000 (30 minutes) */
	uint32_t timeOut; /**< Time of zero contact from a peer before timeout. The default is 5400000 (90 minutes) */
	uint16_t sendTimeOut; /**< Time to wait for a socket to be ready to write before a timeout. */
	uint16_t recvTimeOut; /**< When receiving data after the initial response, the time to wait for the following data before timeout. */
	uint16_t responseTimeOut; /**< Time to wait for a peer to respond to a request before timeout.  */
	uint16_t connectionTimeOut; /**< Time to wait for a socket to connect before timeout. */
	CBByteArray * alternativeMessages; /**< Alternative messages to accept. This should be the 12 byte command names each after another with nothing between. Pass NULL for no alternative message types. */
	uint32_t * altMaxSizes; /**< Sizes for the alternative messages. Will be freed by this object, so malloc this and give it to this object. Send in NULL for a default CB_BLOCK_MAX_SIZE. */
	CBDepObject listeningSocketIPv4; /**< The id of a listening socket on the IPv4 network. */
	CBDepObject listeningSocketIPv6; /**< The id of a listening socket on the IPv6 network. */
	bool isListeningIPv4; /**< True when listening for incomming connections on the IPv4 network. False when not. */
	bool isListeningIPv6; /**< True when listening for incomming connections on the IPv6 network. False when not. */
	CBDepObject eventLoop; /**< Socket event loop */
	CBDepObject acceptEventIPv4; /**< Event for accepting connections on IPv4 */
	CBDepObject acceptEventIPv6; /**< Event for accepting connections on IPv6 */
	uint64_t nonce; /**< Value sent in version messages to check for connections to self */
	CBDepObject pingTimer; /**< Timer for ping event */
	bool isPinging; /**< True when pings are being made. */
	bool isStarted; /**< True if the CBNetworkCommunicator is running. */
	bool stoppedListening; /**< True if listening was stopped because there are too many connections */
	uint64_t pendingIP; /**< A 64 bit integer to create placeholder IPs so that peers which have un-associated addresses are given somehing to make them unique until they get a real IP associated with them. */
	CBIPType reachability; /**< Bitfield for reachable address types */
	CBDepObject addrStorage; /**< The object for address storage. If not 0 and if the CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY flag is given, broadcast addresses will be recorded into the storage. */
	bool useAddrStorage; /**< Set to true if using addrStorage */
	CBAssociativeArray relayedAddrs; /**< An array of the relayed addresses in a 24 hour period, in which the array is cleared. */
	uint64_t relayedAddrsLastClear; /**< The time relayedAddrs was last cleared */
	void (*onPeerConnection)(void *, void *); /**< Callback for when a peer connection has been established. The first argument is the CBNetworkCommunicator and the second is the peer. */
	void (*onPeerDisconnection)(void *, void *); /**< Callback for when a peer has been disconnected. The first argument is the CBNetworkCommunicator and the second is the peer. */
	void (*onTimeOut)(void *, void *, CBTimeOutType); /**< Timeout event callback with a void pointer argument for the CBNetworkCommunicator and then CBNetworkAddress. The callback should return as quickly as possible. Use threads for operations that would otherwise delay the event loop for too long. The first argument is the CBNetworkCommunicator responsible for the timeout. The second argument is the peer with the timeout. Lastly there is the CBTimeOutType */
	CBOnMessageReceivedAction (*onMessageReceived)(void *, void *); /**< The callback for when a message has been received from a peer. The first argument is the CBNetworkCommunicator responsible for receiving the message. The second argument is the CBNetworkAddress peer the message was received from. Return the action that should be done after returning. Access the message by the "receive" feild in the CBNetworkAddress peer. Lookup the type of the message and then cast and/or handle the message approriately. The alternative message bytes can be found in the peer's "alternativeTypeBytes" field. Do not delay the thread for very long. */
	void (*onNetworkError)(void *); /**< Called when both IPv4 and IPv6 fails. Has an argument for the network communicator. */
} CBNetworkCommunicator;

/**
 @brief Creates a new CBNetworkCommunicator object.
 @returns A new CBNetworkCommunicator object.
 */
CBNetworkCommunicator * CBNewNetworkCommunicator(void);

/**
 @brief Initialises a CBNetworkCommunicator object
 @param self The CBNetworkCommunicator object to initialise
 */
void CBInitNetworkCommunicator(CBNetworkCommunicator * self);

/**
 @brief Release and free all of the objects stored by a CBNetworkCommunicator object.
 @param self The CBNetworkCommunicator object to destroy.
 */
void CBDestroyNetworkCommunicator(void * self);
/**
 @brief Frees a CBNetworkCommunicator object and also calls CBDestroyNetworkCommunicator.
 @param self The CBNetworkCommunicator object to free.
 */
void CBFreeNetworkCommunicator(void * self);

//  Functions

/**
 @brief Accepts an incomming connection.
 @param vself The CBNetworkCommunicator object.
 @param socket The listening socket for accepting a connection.
 */
void CBNetworkCommunicatorAcceptConnection(void * vself, CBDepObject socket);
/**
 @brief Returns true if it is beleived the network address can be connected to, otherwise false.
 @param self The CBNetworkCommunicator object.
 @param addr The CBNetworkAddress.
 @returns true if it is beleived the peer can be connected to, otherwise false.
 */
bool CBNetworkCommunicatorCanConnect(CBNetworkCommunicator * self, CBNetworkAddress * addr);
/**
 @brief Connects to a peer. This peer will be added to the peer list if it connects correctly.
 @param self The CBNetworkCommunicator object.
 @param peer The peer to connect to.
 @returns CB_CONNECT_OK if successful. CB_CONNECT_NO_SUPPORT if the IP version is not supported. CB_CONNECT_BAD if the connection failed and the address will be penalised. CB_CONNECT_FAIL if the connection failed but the address will not be penalised.
 */
CBConnectReturn CBNetworkCommunicatorConnect(CBNetworkCommunicator * self, CBPeer * peer);
/**
 @brief Callback for the connection to a peer.
 @param vself The CBNetworkCommunicator object.
 @param vpeer The CBPeer that connected.
 */
void CBNetworkCommunicatorDidConnect(void * vself, void * vpeer);
/**
 @brief Disconnects a peer.
 @param self The CBNetworkCommunicator object.
 @param peer The peer.
 @param penalty Penalty to the score of the address.
 @param stopping If true, do not call "onNetworkError" or remove the peer from the address manager because the CBNetworkCommunicator is stopping.
 */
void CBNetworkCommunicatorDisconnect(CBNetworkCommunicator * self, CBPeer * peer, uint32_t penalty, bool stopping);
/**
 @brief Gets a new version message for this.
 @param self The CBNetworkCommunicator object.
 @param addRecv The CBNetworkAddress of the receipient.
 */
CBVersion * CBNetworkCommunicatorGetVersion(CBNetworkCommunicator * self, CBNetworkAddress * addRecv);
/**
 @brief Determines if an IP type is reachable.
 @param self The CBNetworkCommunicator object.
 @param type The type.
 @returns true if reachable, false if not reachable.
 */
bool CBNetworkCommunicatorIsReachable(CBNetworkCommunicator * self, CBIPType type);
/**
 @brief Called when a peer socket is ready for reading.
 @param vself The CBNetworkCommunicator object.
 @param vpeer The CBPeer index with data to read.
 */
void CBNetworkCommunicatorOnCanReceive(void * vself, void * vpeer);
/**
 @brief Called when a peer socket is ready for writing.
 @param vself The CBNetworkCommunicator object.
 @param vpeer The CBPeer
 */
void CBNetworkCommunicatorOnCanSend(void * vself, void * vpeer);
/**
 @brief Called when a header is received.
 @param self The CBNetworkCommunicator object.
 @param peer The CBPeer.
 */
void CBNetworkCommunicatorOnHeaderRecieved(CBNetworkCommunicator * self, CBPeer * peer);
/**
 @brief Called on an error with the socket event loop. The error event is given with CB_ERROR_NETWORK_COMMUNICATOR_LOOP_FAIL.
 @param vself The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorOnLoopError(void * vself);
/**
 @brief Called when an entire message is received.
 @param self The CBNetworkCommunicator object.
 @param peer The CBPeer.
 */
void CBNetworkCommunicatorOnMessageReceived(CBNetworkCommunicator * self, CBPeer * peer);
/**
 @brief Called on a timeout error. The peer is removed.
 @param vself The CBNetworkCommunicator object.
 @param vpeer The CBPeer index which timedout.
 @param type The type of the timeout
 */
void CBNetworkCommunicatorOnTimeOut(void * vself, void * vpeer, CBTimeOutType type);
/**
 @brief Processes a new received message for auto discovery.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 @returns true if peer should be disconnected, false otherwise.
 */
CBOnMessageReceivedAction CBNetworkCommunicatorProcessMessageAutoDiscovery(CBNetworkCommunicator * self, CBPeer * peer);
/**
 @brief Processes a new received message for auto handshaking.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 @returns true if peer should be disconnected, false otherwise.
 */
CBOnMessageReceivedAction CBNetworkCommunicatorProcessMessageAutoHandshake(CBNetworkCommunicator * self, CBPeer * peer);
/**
 @brief Processes a new received message for auto ping pongs.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 @returns true if peer should be disconnected, false otherwise.
 */
CBOnMessageReceivedAction CBNetworkCommunicatorProcessMessageAutoPingPong(CBNetworkCommunicator * self, CBPeer * peer);
/**
 @brief Sends a message by placing it on the send queue. Will serialise standard messages (unless serialised already) but not alternative messages or alert messages.
 @param self The CBNetworkCommunicator object.
 @param peer The CBPeer.
 @param message The CBMessage to send.
 @param callback The callback for when the send has complete. If NULL, no call is made.
 @returns true if successful, false otherwise.
 */
bool CBNetworkCommunicatorSendMessage(CBNetworkCommunicator * self, CBPeer * peer, CBMessage * message, void (*callback)(void *, void *));
/**
 @brief Sends pings to all connected peers.
 @param self The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorSendPings(void * vself);
/**
 @brief Sets the CBNetworkAddressManager.
 @param self The CBNetworkCommunicator object.
 @param addr The CBNetworkAddressManager
 */
void CBNetworkCommunicatorSetNetworkAddressManager(CBNetworkCommunicator * self, CBNetworkAddressManager * addrMan);
/**
 @brief Sets the alternative messages
 @param self The alternative messages as a CBByteArray with 12 characters per message command, one after the other.
 @param altMaxSizes An allocated memory block of 32 bit integers with the max sizes for the alternative messages.
 @param addr The CBNetworkAddressManager
 */
void CBNetworkCommunicatorSetAlternativeMessages(CBNetworkCommunicator * self, CBByteArray * altMessages, uint32_t * altMaxSizes);
/**
 @brief Sets the IPv4 address for the CBNetworkCommunicator.
 @param self The CBNetworkCommunicator object.
 @param addr The IPv4 address as a CBNetworkAddress.
 */
void CBNetworkCommunicatorSetOurIPv4(CBNetworkCommunicator * self, CBNetworkAddress * ourIPv4);
/**
 @brief Sets the IPv6 address for the CBNetworkCommunicator.
 @param self The CBNetworkCommunicator object.
 @param addr The IPv6 address as a CBNetworkAddress.
 */
void CBNetworkCommunicatorSetOurIPv6(CBNetworkCommunicator * self, CBNetworkAddress * ourIPv6);
/**
 @brief Sets the reachability of an IP type.
 @param self The CBNetworkCommunicator object
 @param type The mask to set the types.
 @param reachable true for setting as reachable, false if not.
 */
void CBNetworkCommunicatorSetReachability(CBNetworkCommunicator * self, CBIPType type, bool reachable);
/**
 @brief Sets the user agent.
 @param self The CBNetworkCommunicator object.
 @param addr The user agent as a CBByteArray.
 */
void CBNetworkCommunicatorSetUserAgent(CBNetworkCommunicator * self, CBByteArray * userAgent);
/**
 @brief Starts a CBNetworkCommunicator by connecting to the peers in the peers list. This starts the socket event loop.
 @param self The CBNetworkCommunicator object.
 @returns true if the CBNetworkCommunicator started successfully, false otherwise.
 */
bool CBNetworkCommunicatorStart(CBNetworkCommunicator * self);
/**
 @brief Causes the CBNetworkCommunicator to begin listening for incomming connections. "isListeningIPv4" and/or "isListeningIPv6" should be set to true if either IPv4 or IPv6 sockets are active.
 @param vself The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorStartListening(CBNetworkCommunicator * self);
/**
 @brief Starts the timer event for sending pings (heartbeat).
 @param vself The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorStartPings(CBNetworkCommunicator * self);
/**
 @brief Closes all connections. This may be neccessary in case of failure in which case CBNetworkCommunicatorStart can be tried again to reconnect to the listed peers.
 @param vself The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorStop(CBNetworkCommunicator * self);
/**
 @brief Stops listening for both IPv6 connections and IPv4 connections.
 @param vself The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorStopListening(CBNetworkCommunicator * self);
/**
 @brief Stops the timer event for sending pings (heartbeat).
 @param vself The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorStopPings(CBNetworkCommunicator * self);
/**
 @brief Looks at the stored addresses and tries to connect to addresses up to the maximum number of allowed connections or as many as there are in the case the maximum number of connections is greater than the number of addresses, plus connected peers.
 @param self The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorTryConnections(CBNetworkCommunicator * self);

#endif
