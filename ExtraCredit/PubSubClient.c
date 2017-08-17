/*********************************************************
* Module Name: PubSub client source 
*
* File Name:    PubSubClient.c
*
* Summary:
*  This file contains the Publish Subscribe Client code.
*
*
*********************************************************/
#include "PubSub.h"
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

volatile sig_atomic_t skipUnsub = 0;

void clientCNTCCode();
void clientUnsubHandler(void);
void CatchAlarm(int ignored);
long getMicroseconds(struct timeval *t);
double convertTimeval(struct timeval *t);
long getTimeSpan(struct timeval *start_time, struct timeval *end_time);
int numberOfTimeOuts=0;
int numberOfTrials;
long totalPing;
int bStop;
int topic;
int topics[MAXTOPICS];

char Version[] = "1.1";   

int main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    struct sockaddr_in fromAddr;     /* Source address of echo */
    unsigned short echoServPort;     /* Echo server port */
    unsigned int fromSize;           /* In-out of address size for recvfrom() */
    char *servIP;                    /* IP address of server */
    char *echoString;                /* String to send to echo server */
    char echoBuffer[ECHOMAX+1];      /* Buffer for receiving echoed string */
    int echoStringLen;               /* Length of string to echo */
    int respStringLen;               /* Length of received response */
    struct hostent *thehost;	     /* Hostent from gethostbyname() */
    double delay;		             /* Iteration delay in seconds */
    int packetSize;                  /* PacketSize*/
    struct timeval *theTime1;
    struct timeval *theTime2;
    struct timeval TV1, TV2;
    struct timeval sentTime, receivedTime;
    int i;
    struct sigaction myaction;
    struct sigaction sig;
    long usec1, usec2, curPing;
    int *seqNumberPtr;
    unsigned int seqNumber = 1;
    unsigned int RxSeqNumber = 1;
    struct timespec reqDelay, remDelay;
    int nIterations;
    long avgPing, loss;
    unsigned int mode;
    //int topic;
    
    unsigned int messageTopic = 0;
    
    theTime1 = &TV1;
    theTime2 = &TV2;

    //Initialize values
    numberOfTimeOuts = 0;
    numberOfTrials = 0;
    totalPing =0;
    bStop = 0;

    if (argc < 4)    /* Test for correct number of arguments */
    {
        fprintf(stderr,"Usage: %s <Server IP> <Server Port> <Mode (Publish = 0)> [<topic>] [<Iteration Delay In Seconds>] [<PacketSize>] [<No. of Iterations>]\n", argv[0]);
        fprintf(stderr, "OR\n");
        fprintf(stderr,"Usage: %s <Server IP> <Server Port> <Mode (Subscribe = 1)> [<topic1> <topic2> ...]\n", argv[0]);
        exit(1);
    }

    signal (SIGINT, clientCNTCCode);

    servIP = argv[1];                       /* First arg: server IP address (dotted quad) */
    echoServPort = atoi(argv[2]);           /* Second arg: server port*/
    mode = atoi(argv[3]);                    /* Third arg: mode */
    /* get info from parameters , or default to defaults if they're not specified */
    
    if(mode == 0){
        if (argc == 4) {
           topic = 1;
           delay = 1;
           packetSize = 32;
           nIterations = 1;
        }
        else if (argc == 5) {
           topic = atoi(argv[4]);
           delay = 1;
           packetSize = 32;
           nIterations = 1;
        }
        else if (argc == 6) {
           topic = atoi(argv[4]);
           delay = atof(argv[5]);
           packetSize = 32;
           nIterations = 1;
        }
        else if (argc == 7) {
           topic = atoi(argv[4]);
           delay = atof(argv[5]);
           packetSize = atoi(argv[6]);
           if (packetSize > ECHOMAX)
             packetSize = ECHOMAX;
           nIterations = 1;
        }
        else if (argc == 8) {
           topic = atoi(argv[4]);
           delay = atof(argv[5]);
           packetSize = atoi(argv[6]);
           if (packetSize > ECHOMAX)
             packetSize = ECHOMAX;
           nIterations = atoi(argv[7]);
        }
    }
    else if(mode == 1){
      packetSize = 40;
      //If topics are given as arguments then put them in an array
      int tmp;
      if(argc > 4){
        for(i=0; i+4<argc; i++){
          tmp = i+4;
          topics[i] = atoi(argv[tmp]);
        }
      }
      //print out topics
      // for(i=0; i<argc-4; i++)
        // printf("Topics[%d] = %d\n", i, topics[i]);
    }

    
    
    myaction.sa_handler = CatchAlarm;
    if (sigfillset(&myaction.sa_mask) < 0)
       DieWithError("sigfillset() failed");

    myaction.sa_flags = 0;

    if (sigaction(SIGALRM, &myaction, 0) < 0)
       DieWithError("sigaction failed for sigalarm");
    
    
    sig.sa_handler = clientUnsubHandler;
    if (sigfillset(&sig.sa_mask) < 0)
      DieWithError("sigfillset() failed");
    
    sig.sa_flags = 0;
    
    if(sigaction(SIGQUIT, &sig, 0) < 0)
      DieWithError("sigaction failed for sigtquit");
      
      
    /* Set up the echo string */

    echoStringLen = packetSize;
    echoString = (char *) echoBuffer;
    

    for (i=0; i<packetSize; i++) {
       echoString[i] = 0;
    }

    seqNumberPtr = (int *)echoString;
    echoString[packetSize-1]='\0';
    
    messageHeader *header = (messageHeader *)echoString;
    header->mode = htons(mode);
    if(mode==0)
      header->topic = htons(topic);

    messageHeader *rcvdHeader;


    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));    /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                 /* Internet addr family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
    
    /* If user gave a dotted decimal address, we need to resolve it  */
    if (echoServAddr.sin_addr.s_addr == -1) {
        thehost = gethostbyname(servIP);
	    echoServAddr.sin_addr.s_addr = *((unsigned long *) thehost->h_addr_list[0]);
    }
    
    echoServAddr.sin_port = htons(echoServPort);     /* Server port */
  //  
  //Publish mode
  //
  if(mode == 0){
    while (nIterations > 0 && bStop != 1) {
  
      *seqNumberPtr = htonl(seqNumber++); 
  
      /* Create a datagram/UDP socket */
      if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
          DieWithError("socket() failed");
  
  
      /* Send the string to the server */
      //printf("UDPEchoClient: Send the string: %s to the server: %s \n", echoString,servIP);
      gettimeofday(theTime1, NULL);
      gettimeofday(&sentTime, NULL);
      header->timeSent = sentTime;
  
      if (sendto(sock, echoString, echoStringLen, 0, (struct sockaddr *)
                 &echoServAddr, sizeof(echoServAddr)) != echoStringLen)
        DieWithError("sendto() sent a different number of bytes than expected");
      
      /* Recv a response */
  
      fromSize = sizeof(fromAddr);
      alarm(2);            //set the timeout for 2 seconds
  
      if ((respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0,
           (struct sockaddr *) &fromAddr, &fromSize)) != echoStringLen) {
          if (errno == EINTR) 
          { 
             printf("Received a  Timeout !!!!!\n"); 
             numberOfTimeOuts++; 
             continue; 
          }
      }
  
      RxSeqNumber = ntohl(*(int *)echoBuffer);
  
      alarm(0);            //clear the timeout 
      gettimeofday(theTime2, NULL);
  
      usec2 = (theTime2->tv_sec) * 1000000 + (theTime2->tv_usec);
      usec1 = (theTime1->tv_sec) * 1000000 + (theTime1->tv_usec);
  
      curPing = (usec2 - usec1);
      printf("Ping(%d): %ld microseconds\n",RxSeqNumber,curPing);
  
      totalPing += curPing;
      numberOfTrials++;
      close(sock);
  
      reqDelay.tv_sec = delay;
      remDelay.tv_nsec = 0;
      nanosleep((const struct timespec*)&reqDelay, &remDelay);
      nIterations--;
    }
  }
  //
  //Subscribe Mode
  //
  else if(mode == 1){
    /* Create a datagram/UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");
        
    //Setup Subscriptions
        
    // IF topics were not yet provided
    if(topics[0] == 0){
      printf("Topic List: \n");
      printf("1 = Topic1\n");
      printf("2 = Topic2\n");
      printf("3 = Topic3\n");
      printf("4 = Topic4\n");
      printf("\n");
      for(i=1; i < MAXTOPICS; i++){
        printf("Input topic to subscribe to (Enter 0 to continue):  ");
        while(scanf("%d", &topic) && topic != 0 && topic > MAXTOPICS){
            printf("\nPlease enter a number from 1-%d:  ", MAXTOPICS);
        }
        if(topic == 0)
          break;
            
        header->topic = htons(topic);
        /* Send the string to the server */
        //printf("UDPEchoClient: Send the string: %s to the server: %s \n", echoString,servIP);
        if (sendto(sock, echoString, echoStringLen, 0, (struct sockaddr *)
                   &echoServAddr, sizeof(echoServAddr)) != echoStringLen)
          DieWithError("sendto() sent a different number of bytes than expected");
          
        // close(sock);
      }
    }
    else{  //Use provided topics to set up subscriptions
        for(i=0; i < MAXTOPICS; i++){
        if(topics[i] == 0)
          break;
            
        header->topic = htons(topics[i]);
        /* Send the string to the server */
        //printf("Subscriber: Subscribed to %d:\n", echoString,servIP);
        if (sendto(sock, echoString, echoStringLen, 0, (struct sockaddr *)
                   &echoServAddr, sizeof(echoServAddr)) != echoStringLen)
          DieWithError("sendto() sent a different number of bytes than expected");
          
        // close(sock);
      }
    }
    numberOfTrials = 0;
    // signal(SIGTSTP, clientUnsubHandler);
    // Recv a response
    while(bStop != 1){
      if(skipUnsub){
        printf("Input 1 to Subscribe or 2 to Unsubscribe\n");
        scanf("%d", &mode);
        if(mode == 1)
          printf("Which topic would you like to Subscribe to?\n");
        else if(mode == 2)
          printf("Which topic would you like to UNsubscribe from?\n");
        else{
          mode = 2;
          printf("Which topic would you like to UNsubscribe from?\n");
        }
        scanf("%d", &topic);
        header->mode = htons(mode);
        header->topic = htons(topic);
        if (sendto(sock, echoString, echoStringLen, 0, (struct sockaddr *)
             &echoServAddr, sizeof(echoServAddr)) != echoStringLen)
          DieWithError("sendto() sent a different number of bytes than expected");
        skipUnsub = 0;
      }
      
      fromSize = sizeof(fromAddr);
      // alarm(2);            //set the timeout for 2 seconds
      
      if ((respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0,
           (struct sockaddr *) &fromAddr, &fromSize)) < 0) {
//            DieWithError("recvfrom() failed");
          printf("Failure on recvfrom, client: %s, errno:%d\n", inet_ntoa(fromAddr.sin_addr),errno);
      }
      else{
        rcvdHeader = (messageHeader *)&echoBuffer;
        messageTopic = ntohs(rcvdHeader->topic);
        if(messageTopic == 0){
          if(numberOfTrials !=0)
            avgPing = (totalPing/numberOfTrials);
          else
            avgPing = 0.0;
          printf("%ld\n", avgPing);
          bStop= 1;
        }
        else{
          numberOfTrials++;
          gettimeofday(&receivedTime, NULL);
          sentTime = rcvdHeader->timeSent;
          curPing = getTimeSpan(&sentTime, &receivedTime);
          totalPing += curPing;
          // printf("Message Received, Topic: %d Size: %d Time: %4.6f\n", messageTopic, respStringLen,
              // ((double)curPing)/1000000);
        }
      }
        
      // alarm(0);
      // close(sock);
    }
    
    
    close(sock);
  }
  
  exit(0);
}

void CatchAlarm(int ignored) { }

void clientCNTCCode() {
  long avgPing, loss;

  bStop = 1;
  if (numberOfTrials != 0) 
    avgPing = (totalPing/numberOfTrials);
  else 
    avgPing = 0;
  if (numberOfTimeOuts != 0) 
    loss = ((numberOfTimeOuts*100)/numberOfTrials);
  else 
    loss = 0;

 // printf("\nAvg Ping: %ld microseconds Loss: %ld Percent\n", avgPing, loss);
  exit(0);
}

void clientUnsubHandler(void){
  // printf("Caught signal in Child.\n");
  skipUnsub = 1;
}

long getMicroseconds(struct timeval *t) {
  return (t->tv_sec) * 1000000 + (t->tv_usec);
}

double convertTimeval(struct timeval *t) {
  return ( t->tv_sec + ( (double) t->tv_usec)/1000000 );
}

long getTimeSpan(struct timeval *start_time, struct timeval *end_time) {
  long usec2 = getMicroseconds(end_time);
  long usec1 = getMicroseconds(start_time);
  return (usec2 - usec1);
}