//
//  CBNetworkCommunicator.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 13/07/2012.
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

/**
 @file
 @brief Used for communicating to other nodes. The network communicator holds function pointers for receiving messages and functions to send messages. The timeouts are in seconds. It is important to understant that a CBNetworkCommunicator does not guarentee thread safety for everything. Thread safety is only given to the "nodes" list. This means it is completely okay to add and remove nodes from multiple threads. Two threads may try to access the list at once such as if the CBNetworkCommunicator receives a socket timeout event and tries to remove an node at the same time as a thread made by a program using cbitcoin tries to add a new node. When using a CBNetworkCommunicator, threading and networking dependencies need to be satisfied, @see CBDependencies.h Inherits CBObject
*/

#ifndef CBNETWORKCOMMUNICATORH
#define CBNETWORKCOMMUNICATORH

//  Includes

#include "CBNode.h"
#include "CBDependencies.h"
#include "CBAddressManager.h"
#include "CBInventoryBroadcast.h"
#include "CBGetBlocks.h"
#include "CBTransaction.h"
#include "CBBlockHeaders.h"
#include "CBPingPong.h"
#include "CBAlert.h"
#include <time.h>

/**
 @brief Structure for CBNetworkCommunicator objects. @see CBNetworkCommunicator.h
*/
typedef struct {
	CBObject base; /**< CBObject base structure */
	uint32_t networkID; /**< The 4 byte id for sending and receiving messages for a given network. */
	CBNetworkCommunicatorFlags flags; /**< Flags for the operation of the CBNetworkCommunicator. */
	int32_t version; /**< Used for automatic handshaking. This version will be advertised to nodes. */
	uint64_t services; /**< Used for automatic handshaking. These services will be advertised */
	CBByteArray * userAgent; /**< Used for automatic handshaking. This user agent will be advertised. */
	int32_t blockHeight; /** Set to the current block height for advertising to nodes during the automated handshake. */
	CBNetworkAddress * ourIPv4; /**< IPv4 network address for us. */
	CBNetworkAddress * ourIPv6; /**< IPv6 network address for us. */
	uint32_t attemptingOrWorkingConnections; /**< All connections being attempted or sucessful */
	uint32_t maxConnections; /**< Maximum number of nodes allowed to connect to. */
	uint32_t numIncommingConnections; /**< Number of incomming connections made */
	uint32_t maxIncommingConnections; /**< Maximum number of incomming connections. */
	CBAddressManager * addresses; /**< All addresses both connected and unconnected */
	uint16_t heartBeat; /**< If the CB_NETWORK_COMMUNICATOR_AUTO_PING flag is set, the CBNetworkCommunicator will send a "ping" message to all nodes after this interval. bitcoin-qt uses 1800 (30 minutes) */
	uint16_t timeOut; /**< Time of zero contact from a node before timeout. bitcoin-qt uses 5400 (90 minutes) */
	uint16_t sendTimeOut; /**< Time to wait for a socket to be ready to write before a timeout. */
	uint16_t recvTimeOut; /**< When receiving data after the initial response, the time to wait for the following data before timeout. */
	uint16_t responseTimeOut; /**< Time to wait for a node to respond to a request before timeout.  */
	uint16_t connectionTimeOut; /**< Time to wait for a socket to connect before timeout. */
	CBByteArray * alternativeMessages; /**< Alternative messages to accept. This should be the 12 byte command names each after another with nothing between. */
	uint32_t * altMaxSizes; /**< Sizes for the alternative messages. Will be freed by this object, so malloc this and give it to this object. Send in NULL for a default CB_BLOCK_MAX_SIZE. */
	uint64_t listeningSocketIPv4; /**< The id of a listening socket on the IPv4 network. */
	uint64_t listeningSocketIPv6; /**< The id of a listening socket on the IPv6 network. */
	bool isListeningIPv4; /**< True when listening for incomming connections on the IPv4 network. False when not. */
	bool isListeningIPv6; /**< True when listening for incomming connections on the IPv6 network. False when not. */
	uint64_t eventLoop; /**< Socket event loop */
	uint64_t acceptEventIPv4; /**< Event for accepting connections on IPv4 */
	uint64_t acceptEventIPv6; /**< Event for accepting connections on IPv6 */
	uint64_t nounce; /**< Value sent in version messages to check for connections to self */
	uint64_t lastPing; /**< Time last ping was sent */
	CBEvents * events; /**< Events. */
	bool isStarted; /**< True if the CBNetworkCommunicator is running. */
	void * callbackHandler; /**< Sent to event callbacks */
} CBNetworkCommunicator;

/**
 @brief Creates a new CBNetworkCommunicator object.
 @returns A new CBNetworkCommunicator object.
 */
CBNetworkCommunicator * CBNewNetworkCommunicator(CBEvents * events);

/**
 @brief Gets a CBNetworkCommunicator from another object. Use this to avoid casts.
 @param self The object to obtain the CBNetworkCommunicator from.
 @returns The CBNetworkCommunicator object.
 */
CBNetworkCommunicator * CBGetNetworkCommunicator(void * self);

/**
 @brief Initialises a CBNetworkCommunicator object
 @param self The CBNetworkCommunicator object to initialise
 @returns true on success, false on failure.
 */
bool CBInitNetworkCommunicator(CBNetworkCommunicator * self,CBEvents * events);

/**
 @brief Frees a CBNetworkCommunicator object.
 @param self The CBNetworkCommunicator object to free.
 */
void CBFreeNetworkCommunicator(void * self);
 
//  Functions

/**
 @brief Accepts an incomming connection.
 @param vself The CBNetworkCommunicator object.
 @param socket The listening socket for accepting a connection.
 @param IPv6 True if an IPv6 connection, false if an IPv4 connection.
 */
void CBNetworkCommunicatorAcceptConnection(void * vself,uint64_t socket,bool IPv6);
/**
 @brief Accepts an incomming IPv4 connection.
 @param vself The CBNetworkCommunicator object.
 @param socket The listening socket for accepting a connection.
 */
void CBNetworkCommunicatorAcceptConnectionIPv4(void * vself,uint64_t socket);
/**
 @brief Accepts an incomming IPv6 connection.
 @param vself The CBNetworkCommunicator object.
 @param socket The listening socket for accepting a connection.
 */
void CBNetworkCommunicatorAcceptConnectionIPv6(void * vself,uint64_t socket);
/**
 @brief Returns true if it is beleived the network address can be connected to, otherwise false.
 @param self The CBNetworkCommunicator object.
 @param addr The CBNetworkAddress.
 @returns true if it is beleived the node can be connected to, otherwise false.
 */
bool CBNetworkCommunicatorCanConnect(CBNetworkCommunicator * self,CBNetworkAddress * addr);
/**
 @brief Connects to a node. This node will be added to the node list if it connects correctly.
 @param self The CBNetworkCommunicator object.
 @param node The node to connect to.
 @returns CB_CONNECT_OK if successful. CB_CONNECT_NO_SUPPORT if the IP version is not supported. CB_CONNECT_BAD if the connection failed and the address will be penalised. CB_CONNECT_FAIL if the connection failed but the address will not be penalised.
 */
CBConnectReturn CBNetworkCommunicatorConnect(CBNetworkCommunicator * self,CBNode * node);
/**
 @brief Callback for the connection to a node.
 @param vself The CBNetworkCommunicator object.
 @param vnode The CBNode that connected.
 */
void CBNetworkCommunicatorDidConnect(void * vself,void * vnode);
/**
 @brief Disconnects a node.
 @param self The CBNetworkCommunicator object.
 @param node The node.
 @param penalty Penalty to the score of the address.
 */
void CBNetworkCommunicatorDisconnect(CBNetworkCommunicator * self,CBNode * node,u_int16_t penalty);
/**
 @brief Gets a new version message for this.
 @param self The CBNetworkCommunicator object.
 @param addRecv The CBNetworkAddress of the receipient.
 */
CBVersion * CBNetworkCommunicatorGetVersion(CBNetworkCommunicator * self,CBNetworkAddress * addRecv);
/**
 @brief Processes a new received message for auto discovery.
 @param self The CBNetworkCommunicator object.
 @param node The node
 @returns true if node should be disconnected, false otherwise.
 */
bool CBNetworkCommunicatorProcessMessageAutoDiscovery(CBNetworkCommunicator * self,CBNode * node);
/**
 @brief Processes a new received message for auto handshaking.
 @param self The CBNetworkCommunicator object.
 @param node The node
 @returns true if node should be disconnected, false otherwise.
 */
bool CBNetworkCommunicatorProcessMessageAutoHandshake(CBNetworkCommunicator * self,CBNode * node);
/**
 @brief Called when a node socket is ready for reading.
 @param vself The CBNetworkCommunicator object.
 @param vnode The CBNode index with data to read.
 */
void CBNetworkCommunicatorOnCanReceive(void * vself,void * vnode);
/**
 @brief Called when a node socket is ready for writing.
 @param vself The CBNetworkCommunicator object.
 @param vnode The CBNode
 */
void CBNetworkCommunicatorOnCanSend(void * vself,void * vnode);
/**
 @brief Called when a header is received.
 @param self The CBNetworkCommunicator object.
 @param node The CBNode.
 @returns true if OK or false if the node has been disconnected.
 */
bool CBNetworkCommunicatorOnHeaderRecieved(CBNetworkCommunicator * self,CBNode * node);
/**
 @brief Called on an error with the socket event loop. The error event is given with CB_ERROR_NETWORK_COMMUNICATOR_LOOP_FAIL.  
 @param vself The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorOnLoopError(void * vself);
/**
 @brief Called when an entire message is received.
 @param self The CBNetworkCommunicator object.
 @param node The CBNode.
 */
void CBNetworkCommunicatorOnMessageReceived(CBNetworkCommunicator * self,CBNode * node);
/**
 @brief Called on a timeout error. The node is removed.
 @param vself The CBNetworkCommunicator object.
 @param vnode The CBNode index which timedout.
 @param type The type of the timeout
 */
void CBNetworkCommunicatorOnTimeOut(void * vself,void * vnode,CBTimeOutType type);
/**
 @brief Sends a message by placing it on the send queue. Will serialise standard messages (unless serialised already) but not alternative messages or alert messages.
 @param self The CBNetworkCommunicator object.
 @param node The CBNode.
 @param message The CBMessage to send.
 @returns true if successful, false otherwise.
 */
bool CBNetworkCommunicatorSendMessage(CBNetworkCommunicator * self,CBNode * node,CBMessage * message);
/**
 @brief Sets the CBAddressManager.
 @param self The CBNetworkCommunicator object.
 @param addr The CBAddressManager
 */
void CBNetworkCommunicatorSetAddressManager(CBNetworkCommunicator * self,CBAddressManager * addrMan);
/**
 @brief Sets the alternative messages
 @param self The alternative messages as a CBByteArray with 12 characters per message command, one after the other.
 @param altMaxSizes An allocated memory block of 32 bit integers with the max sizes for the alternative messages.
 @param addr The CBAddressManager
 */
void CBNetworkCommunicatorSetAlternativeMessages(CBNetworkCommunicator * self,CBByteArray * altMessages,uint32_t * altMaxSizes);
/**
 @brief Sets the IPv4 address for the CBNetworkCommunicator.
 @param self The CBNetworkCommunicator object.
 @param addr The IPv4 address as a CBNetworkAddress.
 */
void CBNetworkCommunicatorSetOurIPv4(CBNetworkCommunicator * self,CBNetworkAddress * ourIPv4);
/**
 @brief Sets the IPv6 address for the CBNetworkCommunicator.
 @param self The CBNetworkCommunicator object.
 @param addr The IPv6 address as a CBNetworkAddress.
 */
void CBNetworkCommunicatorSetOurIPv6(CBNetworkCommunicator * self,CBNetworkAddress * ourIPv6);
/**
 @brief Sets the user agent.
 @param self The CBNetworkCommunicator object.
 @param addr The user agent as a CBByteArray.
 */
void CBNetworkCommunicatorSetUserAgent(CBNetworkCommunicator * self,CBByteArray * userAgent);
/**
 @brief Starts a CBNetworkCommunicator by connecting to the nodes in the nodes list. This starts the socket event loop.
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
 @brief Closes all connections. This may be neccessary in case of failure in which case CBNetworkCommunicatorStart can be tried again to reconnect to the listed nodes.
 @param vself The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorStop(CBNetworkCommunicator * self);
/**
 @brief Stops listening for both IPv6 connections and IPv4 connections.
 @param vself The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorStopListening(CBNetworkCommunicator * self);
/**
 @brief Looks at the stored addresses and tries to connect to addresses up to the maximum number of allowed connections or as many as there are in the case the maximum number of connections is greater than the number of addresses, plus connected nodes.
 @param self The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorTryConnections(CBNetworkCommunicator * self);

#endif
