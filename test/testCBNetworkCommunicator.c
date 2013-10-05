//
//  testCBNetworkCommunicator.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 10/08/2012.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include <stdio.h>
#include "CBNetworkCommunicator.h"
#include "CBLibEventSockets.h"
#include <event2/thread.h>
#include <time.h>
#include "stdarg.h"

void CBLogError(char * format, ...);
void CBLogError(char * format, ...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

typedef enum{
	GOTVERSION = 1, 
	GOTACK = 2, 
	GOTPING = 4, 
	GOTPONG = 8, 
	GOTGETADDR = 16,
	COMPLETE = 32,
}TesterProgress;

typedef struct{
	TesterProgress prog[7];
	CBPeer * peerToProg[7];
	uint8_t progNum;
	int complete;
	int addrComplete;
	CBNetworkCommunicator * comms[3];
	pthread_mutex_t testingMutex;
}Tester;

uint64_t CBGetMilliseconds(void){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void onTimeOut(CBNetworkCommunicator * comm, void * peer, CBTimeOutType type);
void onTimeOut(CBNetworkCommunicator * comm, void * peer, CBTimeOutType type){
	switch (type) {
		case CB_TIMEOUT_CONNECT:
			printf("TIMEOUT FAIL: CONNECT\n");
			break;
		case CB_TIMEOUT_NO_DATA:
			printf("TIMEOUT FAIL: NO DATA\n");
			break;
		case CB_TIMEOUT_RECEIVE:
			printf("TIMEOUT FAIL: RECEIVE\n");
			break;
		case CB_TIMEOUT_RESPONSE:
			printf("TIMEOUT FAIL: RESPONSE\n");
			break;
		case CB_TIMEOUT_SEND:
			printf("TIMEOUT FAIL: SEND\n");
			break;
	}
	CBNetworkCommunicatorStop((CBNetworkCommunicator *)comm);
}

void stop(void * comm);
void stop(void * comm){
	CBNetworkCommunicatorStop(comm);
}

bool acceptType(CBNetworkCommunicator *, CBMessageType);
bool acceptType(CBNetworkCommunicator * comm, CBMessageType type){
	return true;
}

Tester tester;

CBOnMessageReceivedAction onMessageReceived(CBNetworkCommunicator * comm, CBPeer * peer);
CBOnMessageReceivedAction onMessageReceived(CBNetworkCommunicator * comm, CBPeer * peer){
	pthread_mutex_lock(&tester.testingMutex); // Only one processing of test at a time.
	CBMessage * theMessage = peer->receive;
	// Assign peer to tester progress.
	uint8_t x = 0;
	for (; x < tester.progNum; x++)
		if (tester.peerToProg[x] == peer)
			break;
	if (x == tester.progNum) {
		printf("NEW NODE OBJ: (%s, %p), (%p)\n", (comm->ourIPv4->port == 45562)? "L1" : ((comm->ourIPv4->port == 45563)? "L2" : "CN"), (void *)comm, (void *)peer);
		tester.peerToProg[x] = peer;
		tester.progNum++;
	}
	TesterProgress * prog = tester.prog + x;
	printf("%s received %u from %s (%p) WITH TESTER %i and PROG %i (%p)\n", (comm->ourIPv4->port == 45562)? "L1" : ((comm->ourIPv4->port == 45563)? "L2" : "CN"), theMessage->type, (CBGetNetworkAddress(peer)->port == 45562)? "L1" : ((CBGetNetworkAddress(peer)->port == 45563)? "L2" : ((CBGetNetworkAddress(peer)->port == 45564) ? "CN" : "UK")), (void *)peer, x, *prog, (void *)prog);
	switch (theMessage->type) {
		case CB_MESSAGE_TYPE_VERSION:
			if (! ((peer->handshakeStatus & CB_HANDSHAKE_SENT_VERSION && *prog == GOTACK) || (*prog == 0))) {
				printf("VERSION FAIL\n");
				exit(EXIT_FAILURE);
			}
			if (CBGetVersion(theMessage)->services) {
				printf("VERSION SERVICES FAIL\n");
				exit(EXIT_FAILURE);
			}
			if (CBGetVersion(theMessage)->version != CB_PONG_VERSION) {
				printf("VERSION VERSION FAIL\n");
				exit(EXIT_FAILURE);
			}
			if (memcmp(CBByteArrayGetData(CBGetVersion(theMessage)->userAgent), CB_USER_AGENT_SEGMENT, CBGetVersion(theMessage)->userAgent->length)) {
				printf("VERSION USER AGENT FAIL\n");
				exit(EXIT_FAILURE);
			}
			if (memcmp(CBByteArrayGetData(CBGetVersion(theMessage)->addSource->ip), (uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 127, 0, 0, 1}, CBGetVersion(theMessage)->addSource->ip->length)) {
				printf("VERSION SOURCE IP FAIL\n");
				exit(EXIT_FAILURE);
			}
			if (memcmp(CBByteArrayGetData(CBGetVersion(theMessage)->addRecv->ip), (uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 127, 0, 0, 1}, CBGetVersion(theMessage)->addRecv->ip->length)) {
				printf("VERSION RECEIVE IP FAIL\n");
				exit(EXIT_FAILURE);
			}
			*prog |= GOTVERSION;
			break;
		case CB_MESSAGE_TYPE_VERACK:
			if ((! (peer->handshakeStatus & CB_HANDSHAKE_SENT_VERSION) || peer->handshakeStatus & CB_HANDSHAKE_GOT_ACK)) {
				printf("VERACK FAIL\n");
				exit(EXIT_FAILURE);
			}
			*prog |= GOTACK;
			break;
		case CB_MESSAGE_TYPE_PING:
			if (! (*prog & GOTVERSION && *prog & GOTACK)) {
				printf("PING FAIL\n");
				exit(EXIT_FAILURE);
			}
			CBGetPingPong(theMessage)->ID = CBGetPingPong(theMessage)->ID; // Test access to ID.
			*prog |= GOTPING;
			break;
		case CB_MESSAGE_TYPE_PONG:
			if (! (*prog & GOTVERSION && *prog & GOTACK)) {
				printf("PONG FAIL\n");
				exit(EXIT_FAILURE);
			}
			CBGetPingPong(theMessage)->ID = CBGetPingPong(theMessage)->ID; // Test access to ID.
			// At least one of the pongs worked.
			*prog |= GOTPONG;
			break;
		case CB_MESSAGE_TYPE_GETADDR:
			if (! (*prog & GOTVERSION && *prog & GOTACK)) {
				printf("GET ADDR FAIL\n");
				exit(EXIT_FAILURE);
			}
			*prog |= GOTGETADDR;
			break;
		case CB_MESSAGE_TYPE_ADDR:
			if (! (*prog & GOTVERSION && *prog & GOTACK)) {
				printf("ADDR FAIL\n");
				exit(EXIT_FAILURE);
			}
			tester.addrComplete++;
			break;
		default:
			printf("MESSAGE FAIL\n");
			exit(EXIT_FAILURE);
			break;
	}
	if (*prog == (GOTVERSION | GOTACK | GOTPING | GOTPONG | GOTGETADDR)) {
		*prog |= COMPLETE;
		tester.complete++;
	}
	printf("COMPLETION: %i - %i\n", tester.addrComplete, tester.complete);
	if (tester.addrComplete > 7) {
		// addrComplete can be seven in the case that one of the listening nodes has connected to the other and then the connecting node asks the other node what addresses it has and it has the other node.
		printf("ADDR COMPLETE FAIL\n");
		exit(EXIT_FAILURE);
	}
	if (tester.complete == 6) {
		if (tester.addrComplete > 4) {
			// Usually addrComplete will be 6 but sometimes addresses are not sent in response to getaddr when the address is being connected to. In reality this is not a problem, as it is seldom an address will be in a connecting state.
			// Completed testing
			printf("DONE\n");
			printf("STOPPING COMM L1\n");
			CBRunOnEventLoop(tester.comms[0]->eventLoop, stop, tester.comms[0]);
			printf("STOPPING COMM L2\n");
			CBRunOnEventLoop(tester.comms[1]->eventLoop, stop, tester.comms[1]);
			printf("STOPPING COMM CN\n");
			CBRunOnEventLoop(tester.comms[2]->eventLoop, stop, tester.comms[2]);
			return CB_MESSAGE_ACTION_RETURN;
		}else{
			printf("ADDR COMPLETE DURING COMPLETE FAIL\n");
			exit(EXIT_FAILURE);
		}
	}
	pthread_mutex_unlock(&tester.testingMutex);
	return CB_MESSAGE_ACTION_CONTINUE;
}
void onNetworkError(CBNetworkCommunicator * comm);
void onNetworkError(CBNetworkCommunicator * comm){
	printf("DID LOSE LAST NODE\n");
	exit(EXIT_FAILURE);
}
void onBadTime(void * foo);
void onBadTime(void * foo){
	printf("BAD TIME FAIL\n");
	exit(EXIT_FAILURE);
}
void onPeerWhatever(CBNetworkCommunicator * foo, CBPeer * bar);
void onPeerWhatever(CBNetworkCommunicator * foo, CBPeer * bar){
	return;
}

int main(){
	memset(&tester, 0, sizeof(tester));
	pthread_mutex_init(&tester.testingMutex, NULL);
	evthread_use_pthreads();
	// Create three CBNetworkCommunicators and connect over the loopback address. Two will listen, one will connect. Test auto handshake, auto ping and auto discovery.
	CBByteArray * loopBack = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 127, 0, 0, 1}, 16);
	CBByteArray * loopBack2 = CBByteArrayCopy(loopBack); // Do not use in more than one thread.
	CBNetworkAddress * addrListen = CBNewNetworkAddress(0, loopBack, 45562, 0, false);
	CBNetworkAddress * addrListenB = CBNewNetworkAddress(0, loopBack2, 45562, 0, true); // Use in connector thread.
	CBNetworkAddress * addrListen2 = CBNewNetworkAddress(0, loopBack, 45563, 0, false);
	CBNetworkAddress * addrListen2B = CBNewNetworkAddress(0, loopBack2, 45563, 0, true); // Use in connector thread.
	CBNetworkAddress * addrConnect = CBNewNetworkAddress(0, loopBack, 45564, 0, false); // Different port over loopback to seperate the CBNetworkCommunicators.
	CBReleaseObject(loopBack);
	CBReleaseObject(loopBack2);
	CBByteArray * userAgent = CBNewByteArrayFromString(CB_USER_AGENT_SEGMENT, false);
	CBByteArray * userAgent2 = CBNewByteArrayFromString(CB_USER_AGENT_SEGMENT, false);
	CBByteArray * userAgent3 = CBNewByteArrayFromString(CB_USER_AGENT_SEGMENT, false);
	// First listening CBNetworkCommunicator setup.
	CBNetworkAddressManager * addrManListen = CBNewNetworkAddressManager(onBadTime);
	addrManListen->maxAddressesInBucket = 2;
	CBNetworkCommunicatorCallbacks callbacks = {
		onPeerWhatever,
		onPeerWhatever,
		onTimeOut,
		acceptType,
		onMessageReceived,
		onNetworkError
	};
	CBNetworkCommunicator * commListen = CBNewNetworkCommunicator(callbacks);
	CBNetworkCommunicatorSetReachability(commListen, CB_IP_IPv4 | CB_IP_LOCAL, true);
	addrManListen->callbackHandler = commListen;
	commListen->networkID = CB_PRODUCTION_NETWORK_BYTES;
	commListen->flags = CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING | CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY;
	commListen->version = CB_PONG_VERSION;
	commListen->maxConnections = 3;
	commListen->maxIncommingConnections = 3; // One for connector, one for the other listener and an extra so that we continue to share our address.
	commListen->heartBeat = 1000;
	commListen->timeOut = 2000;
	CBNetworkCommunicatorSetAlternativeMessages(commListen, NULL, NULL);
	CBNetworkCommunicatorSetNetworkAddressManager(commListen, addrManListen);
	CBNetworkCommunicatorSetUserAgent(commListen, userAgent);
	CBNetworkCommunicatorSetOurIPv4(commListen, addrListen);
	// Second listening CBNetworkCommunicator setup.
	CBNetworkAddressManager * addrManListen2 = CBNewNetworkAddressManager(onBadTime);
	addrManListen2->maxAddressesInBucket = 2;
	CBNetworkCommunicator * commListen2 = CBNewNetworkCommunicator(callbacks);
	CBNetworkCommunicatorSetReachability(commListen2, CB_IP_IPv4 | CB_IP_LOCAL, true);
	addrManListen2->callbackHandler = commListen2;
	commListen2->networkID = CB_PRODUCTION_NETWORK_BYTES;
	commListen2->flags = CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING | CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY;
	commListen2->version = CB_PONG_VERSION;
	commListen2->maxConnections = 3;
	commListen2->maxIncommingConnections = 3;
	commListen2->heartBeat = 1000;
	commListen2->timeOut = 2000;
	CBNetworkCommunicatorSetAlternativeMessages(commListen2, NULL, NULL);
	CBNetworkCommunicatorSetNetworkAddressManager(commListen2, addrManListen2);
	CBNetworkCommunicatorSetUserAgent(commListen2, userAgent2);
	CBNetworkCommunicatorSetOurIPv4(commListen2, addrListen2);
	// Connecting CBNetworkCommunicator setup.
	CBNetworkAddressManager * addrManConnect = CBNewNetworkAddressManager(onBadTime);
	addrManConnect->maxAddressesInBucket = 2;
	// We are going to connect to both listing CBNetworkCommunicators.
	CBNetworkAddressManagerAddAddress(addrManConnect, addrListenB);
	CBNetworkAddressManagerAddAddress(addrManConnect, addrListen2B);
	CBNetworkCommunicator * commConnect = CBNewNetworkCommunicator(callbacks);
	CBNetworkCommunicatorSetReachability(commConnect, CB_IP_IPv4 | CB_IP_LOCAL, true);
	addrManConnect->callbackHandler = commConnect;
	commConnect->networkID = CB_PRODUCTION_NETWORK_BYTES;
	commConnect->flags = CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING | CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY;
	commConnect->version = CB_PONG_VERSION;
	commConnect->maxConnections = 2;
	commConnect->maxIncommingConnections = 0;
	commConnect->heartBeat = 1000;
	commConnect->timeOut = 2000;
	CBNetworkCommunicatorSetAlternativeMessages(commConnect, NULL, NULL);
	CBNetworkCommunicatorSetNetworkAddressManager(commConnect, addrManConnect);
	CBNetworkCommunicatorSetUserAgent(commConnect, userAgent3);
	CBNetworkCommunicatorSetOurIPv4(commConnect, addrConnect);
	// Release objects
	CBReleaseObject(userAgent);
	// Give tester communicators
	tester.comms[0] = commListen;
	tester.comms[1] = commListen2;
	tester.comms[2] = commConnect;
	// Start listening on first listener. Can start listening on this thread since the event loop will not be doing anything.
	CBNetworkCommunicatorStart(commListen);
	pthread_t listenThread = ((CBEventLoop *) commListen->eventLoop.ptr)->loopThread;
	CBNetworkCommunicatorStartListening(commListen);
	if (! commListen->isListeningIPv4) {
		printf("FIRST LISTEN FAIL\n");
		exit(EXIT_FAILURE);
	}
	// Start listening on second listener.
	CBNetworkCommunicatorStart(commListen2);
	pthread_t listen2Thread = ((CBEventLoop *) commListen2->eventLoop.ptr)->loopThread;
	CBNetworkCommunicatorStartListening(commListen2);
	if (! commListen2->isListeningIPv4) {
		printf("SECOND LISTEN FAIL\n");
		exit(EXIT_FAILURE);
	}
	// Start connection
	CBNetworkCommunicatorStart(commConnect);
	pthread_t connectThread = ((CBEventLoop *) commConnect->eventLoop.ptr)->loopThread;
	CBNetworkCommunicatorTryConnections(commConnect);
	// Wait until the network loop ends for all CBNetworkCommunicators.
	pthread_join(listenThread, NULL);
	pthread_join(listen2Thread, NULL);
	pthread_join(connectThread, NULL);
	// Check addresses in the first listening CBNetworkCommunicator
	if (commListen->addresses->addrNum != 1) { // Only L2 should go back to addresses. CN is private
		printf("ADDRESS DISCOVERY LISTEN ONE ADDR NUM FAIL %i != 1\n", commListen->addresses->addrNum);
		exit(EXIT_FAILURE);
	}
	if(! CBNetworkAddressManagerGotNetworkAddress(commListen->addresses, addrListen2)){
		printf("ADDRESS DISCOVERY LISTEN ONE LISTEN TWO FAIL\n");
		exit(EXIT_FAILURE);
	}
	// Check the addresses in the second.
	if (commListen->addresses->addrNum != 1) {
		printf("ADDRESS DISCOVERY LISTEN TWO ADDR NUM FAIL\n");
		exit(EXIT_FAILURE);
	}
	if (! CBNetworkAddressManagerGotNetworkAddress(commListen2->addresses, addrListen)){
		printf("ADDRESS DISCOVERY LISTEN TWO LISTEN ONE FAIL\n");
		exit(EXIT_FAILURE);
	}
	// And lastly the connecting CBNetworkCommunicator
	if (commConnect->addresses->addrNum != 2) {
		printf("ADDRESS DISCOVERY CONNECT ADDR NUM FAIL\n");
		exit(EXIT_FAILURE);
	}
	addrListen->bucketSet = false;
	if (! CBNetworkAddressManagerGotNetworkAddress(commConnect->addresses, addrListen)){
		printf("ADDRESS DISCOVERY CONNECT LISTEN ONE FAIL\n");
		exit(EXIT_FAILURE);
	}
	addrListen2->bucketSet = false;
	if (! CBNetworkAddressManagerGotNetworkAddress(commConnect->addresses, addrListen2)){
		printf("ADDRESS DISCOVERY CONNECT LISTEN TWO FAIL\n");
		exit(EXIT_FAILURE);
	}
	// Release all final objects.
	CBReleaseObject(addrListen);
	CBReleaseObject(addrListen2);
	CBReleaseObject(addrConnect);
	CBReleaseObject(commListen);
	CBReleaseObject(commListen2);
	CBReleaseObject(commConnect);
	pthread_mutex_destroy(&tester.testingMutex);
	return EXIT_SUCCESS;
}
