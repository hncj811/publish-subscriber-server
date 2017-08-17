/*********************************************************
*
* Module Name: PubSub client/server header file
*
* File Name:    PubSub.h	
*
* Summary:
*  This file contains common stuff for the client and server
*
* Revisions:
*
*********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>     /* for memset() */
#include <netinet/in.h> /* for in_addr */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <unistd.h>     /* for close() */
#include <time.h>
#include <signal.h>

#define ECHOMAX 10000     /* Longest string to echo */
#define MAXTOPICS 4       /* Max amount of topics */

#ifndef LINUX
#define INADDR_NONE  0xffffffff
#endif

void DieWithError(char *errorMessage);  /* External error handling function */

typedef struct {
	unsigned int mode;  // 0 publisher, 1 subscriber 
	unsigned int topic;  // ranging from 0-3
	struct timeval timeSent;
}messageHeader;