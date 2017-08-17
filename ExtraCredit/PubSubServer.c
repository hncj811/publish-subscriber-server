/*********************************************************
*
* Module Name: PubSub Server
*
* File Name:    PubSubServer.c	
* 
* Based on UDPEchoServer.c from Dr. Jim Martin
*
* Summary:
*  This file contains the Publish Subscribe Server code.
* 
* 
*********************************************************/
#include "PubSub.h"

void DieWithError(char *errorMessage);  /* External error handling function */

void serverCNTCCode();
int bStop;
int sendClose = 0;
long totalPing;

char Version[] = "1.1";   

typedef struct subClient{
    struct in_addr clientIP;
    unsigned short clientPort;
    struct sockaddr_in clientAddr;
    int topics[MAXTOPICS];
    // struct subClient *prev;
    struct subClient *next;
}subClient;  // subscriber client linked list




// multiple list version

subClient *findClient2(struct sockaddr_in clientAddr, subClient *ls) {
    subClient *s = ls;
    while (s != NULL){
        if(s->clientAddr.sin_addr.s_addr == clientAddr.sin_addr.s_addr && s->clientAddr.sin_port == clientAddr.sin_port)
            return s;
        else
            s = s->next;
    }
    return s;
}


// multiple list version either finds subClient and returns or inserts subClient and returns

subClient *getClient2(struct sockaddr_in clientAddr, subClient *ls) {
  subClient *s = findClient2(clientAddr, ls);
  // If matching subClient is not found...
  if (s == NULL) {
    // ...add new subClient
    s = malloc(1 * sizeof(subClient));
    s->clientIP = clientAddr.sin_addr;
    s->clientPort = clientAddr.sin_port;
    s->clientAddr = clientAddr;
    s->next = ls; 
    ls = s;  
    
  }
  return s;
}


void printSubClients2(subClient *ls) {
  subClient *toprint = ls;
  printf("\n");
  int i=0; 
  while(i< MAXTOPICS){
    if(i != 0) break; 
    ++i; 
  }
  printf("***Topic %d Subscriber List ***\n", i);
//   printf("cAddr clientPort, durSecs/1000000, bytes numberRxed bps lossrate \n");
  while (toprint != NULL) {
    char *cAddr = inet_ntoa(toprint->clientIP);

    printf("Subscriber: %s %d\n", 
        cAddr, toprint->clientPort);
    
    toprint = toprint->next;
  }
}

void freeList(subClient *ls){
    subClient *toFree; 
    while (toFree != NULL){
        toFree = ls; 
        ls = ls->next; 
        free(toFree); 
    }
}

int main(int argc, char *argv[])
{
    int sock;                        /* Socket */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int cliAddrLen;         /* Length of incoming message */
    char echoBuffer[ECHOMAX];        /* Buffer for echo string */
    unsigned short echoServPort;     /* Server port */
    int recvMsgSize;                 /* Size of received message */
    
    struct sigaction closeAction;
    
    unsigned int clientMode;
    unsigned int clientTopic;
    messageHeader *header;
    
    subClient *ls1 = NULL; 
    subClient *ls2 = NULL;
    subClient *ls3 = NULL; 
    subClient *ls0 = NULL; 
    
    bStop = 0;
    
    if (argc != 2)         /* Test for correct number of parameters */
    {
        fprintf(stderr,"Usage:  %s <SERVER PORT>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);  /* First arg:  local port */

//$A0
    printf("PubSubServer(version:%s): Port:%d\n",(char *)Version,echoServPort);   
    
    
    closeAction.sa_handler = serverCNTCCode;
    if(sigfillset(&closeAction.sa_mask) < 0)
        DieWithError("sigfillset() failed");
    closeAction.sa_flags = 0;
    if(sigaction(SIGQUIT, &closeAction, 0) < 0)
        DieWithError("sigaction failed for sigquit");

    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
//        DieWithError("socket() failed");
      printf("Failure on socket call , errno:%d\n",errno);
    }

    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0) {
//        DieWithError("bind() failed");
          printf("Failure on bind, errno:%d\n",errno);
    }
  
    subClient *currentClient = NULL;
  
    while(bStop != 1)
    {
        /* Set the size of the in-out parameter */
        cliAddrLen = sizeof(echoClntAddr);
        
        /* Block until receive message from a client */
        if ((recvMsgSize = recvfrom(sock, echoBuffer, ECHOMAX, 0,
            (struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0)
        {
//            DieWithError("recvfrom() failed");
          printf("Failure on recvfrom, client: %s, errno:%d\n", inet_ntoa(echoClntAddr.sin_addr),errno);
        }
    
        else{
            header = (messageHeader *)&echoBuffer;
            clientMode = ntohs(header->mode);
            clientTopic = ntohs(header->topic);
            
            if(clientMode == 0){ 
                /* Send received datagram back to the client */
                if (sendto(sock, echoBuffer, recvMsgSize, 0,  
                     (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != recvMsgSize) {
    //            DieWithError("sendto() sent a different number of bytes than expected");
                  printf("Failure on sendTo, client: %s, errno:%d\n", inet_ntoa(echoClntAddr.sin_addr),errno);
                }
                else{
                    printf("Publish client: %s, mode: %d, topic %d\n", 
                        inet_ntoa(echoClntAddr.sin_addr), clientMode, clientTopic);
                }
                subClient *toSend; 
                if (clientTopic== 0)  toSend = ls0;
                else if(clientTopic == 1)  toSend = ls1;
                else if(clientTopic == 2)  toSend = ls2;
                else toSend = ls3;
                while(toSend != NULL){
                 
                    if(sendto(sock, echoBuffer, recvMsgSize, 0,  (struct sockaddr *) &toSend->clientAddr, sizeof(toSend->clientAddr)) !=recvMsgSize){
                                    //            DieWithError("sendto() sent a different number of bytes than expected");
                                    printf("Failure on sendTo, client: %s, errno:%d\n", inet_ntoa(echoClntAddr.sin_addr),errno);
                    }
                    else
                        printf("Message Sent to subscriber: %s, with topic: %d\n", inet_ntoa(toSend->clientAddr.sin_addr), clientTopic);
                    
                    toSend = toSend->next;
                }
            }
            else if(clientMode == 1){  // mode 1 is subscribe
                subClient *toSend; 
                if (clientTopic== 0)  toSend = ls0;
                else if(clientTopic == 1)  toSend = ls1;
                else if(clientTopic == 2)  toSend = ls2;
                else toSend = ls3;
                currentClient = findClient2(echoClntAddr, toSend);
                if(currentClient == NULL){
                    currentClient = getClient2(echoClntAddr, toSend); 
                    currentClient->topics[clientTopic] = clientTopic; 
                    printf("Subscriber client: %s is NOW subscribed to Topic %d\n", 
                    inet_ntoa(echoClntAddr.sin_addr), clientTopic);
                }
                else{
                    printf("Subscriber client: %s is ALREADY subscribed to Topic %d\n", 
                    inet_ntoa(echoClntAddr.sin_addr), clientTopic);
                }
                //printSubClients();
            }
            else if(clientMode == 2){  //mode 2 is unsubscribe
                subClient *toSend; 
                if (clientTopic== 0)  toSend = ls0;
                else if(clientTopic == 1)  toSend = ls1;
                else if(clientTopic == 2)  toSend = ls2;
                else toSend = ls3;
                currentClient = findClient2(echoClntAddr, toSend);
                if(currentClient != NULL ){
                   currentClient->topics[clientTopic] = 0; 
                   subClient *toDelete = toSend; 
                   while(toDelete->next != currentClient) toDelete = toDelete->next; 
                   toDelete->next = currentClient->next; 
                   free(currentClient);
                   printf("Subscriber client: %s is now UNsubscribed to Topic %d\n", 
                    inet_ntoa(echoClntAddr.sin_addr), clientTopic);
                }
                else
                    printf("Subscriber client: %s is ALREADY UNsubscribed to Topic %d\n", 
                     inet_ntoa(echoClntAddr.sin_addr), clientTopic);
                // removeActive(echoClntAddr);
               //
               // printSubClients();
               //
            }
        }
    }
    printf("Sending Close Message to Subscribers\n");
    
    recvMsgSize = 50;
/****************************
*
*I don't think this method will work properly. What happens when a subscriber is subbed to more than one?
*
****************************/
    // memset(&echoBuffer, 0, sizeof(echoBuffer));
    header = (messageHeader *)&echoBuffer;
    header->mode = htons(1);
    header->topic = htons(0);
    subClient *toSend = ls0;
    while(toSend != NULL){
        if(sendto(sock, echoBuffer, recvMsgSize, 0,
            (struct sockaddr *) &toSend->clientAddr, sizeof(toSend->clientAddr)) !=recvMsgSize){
                //            DieWithError("sendto() sent a different number of bytes than expected");
                printf("Failure on sendTo, client: %s, errno:%d\n", inet_ntoa(echoClntAddr.sin_addr),errno);
        }
        else
            printf("Closing subscriber: %s %d\n", inet_ntoa(toSend->clientAddr.sin_addr), toSend->clientPort);
    
        toSend = toSend->next;
    }
    toSend = ls1;
    while(toSend != NULL){
        if(sendto(sock, echoBuffer, recvMsgSize, 0,
            (struct sockaddr *) &toSend->clientAddr, sizeof(toSend->clientAddr)) !=recvMsgSize){
                //            DieWithError("sendto() sent a different number of bytes than expected");
                printf("Failure on sendTo, client: %s, errno:%d\n", inet_ntoa(echoClntAddr.sin_addr),errno);
        }
        else
            printf("Closing subscriber: %s %d\n", inet_ntoa(toSend->clientAddr.sin_addr), toSend->clientPort);
    
        toSend = toSend->next;
    }
    toSend = ls2;
    while(toSend != NULL){
        if(sendto(sock, echoBuffer, recvMsgSize, 0,
            (struct sockaddr *) &toSend->clientAddr, sizeof(toSend->clientAddr)) !=recvMsgSize){
                //            DieWithError("sendto() sent a different number of bytes than expected");
                printf("Failure on sendTo, client: %s, errno:%d\n", inet_ntoa(echoClntAddr.sin_addr),errno);
        }
        else
            printf("Closing subscriber: %s %d\n", inet_ntoa(toSend->clientAddr.sin_addr), toSend->clientPort);
    
        toSend = toSend->next;
    }
    toSend = ls3;
    while(toSend != NULL){
        if(sendto(sock, echoBuffer, recvMsgSize, 0,
            (struct sockaddr *) &toSend->clientAddr, sizeof(toSend->clientAddr)) !=recvMsgSize){
                //            DieWithError("sendto() sent a different number of bytes than expected");
                printf("Failure on sendTo, client: %s, errno:%d\n", inet_ntoa(echoClntAddr.sin_addr),errno);
        }
        else
            printf("Closing subscriber: %s %d\n", inet_ntoa(toSend->clientAddr.sin_addr), toSend->clientPort);
    
        toSend = toSend->next;
    }
    freeList(ls0);
    freeList(ls1); freeList(ls2); freeList(ls3);
    close(sock);
    
    return 0;
}

void serverCNTCCode(){
    bStop = 1;
}
