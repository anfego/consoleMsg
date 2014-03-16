/**************************************************************************************
 FILE NAME:			server.c
 PROGRAMMER:		Ivan Syzonenko
 CLASS:				CSCI 5300
 DUE DATE:			11/30/2013
 INSTRUCTOR:			Dr. Gu
 DESCRIPTION:			Allocates a socket. Waits and prints messages from
					clients. Sends messages to clients
					
					Syntax: server [ port ]

					port - protocol port number to use

					Note: The port argument is optional. If no port is specified,
					the server uses the default given by PROTOPORT.
***************************************************************************************/

#define closesocket close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>	// use thread functions

#include <stdio.h>
#include <string.h>

//
#include "userInfo.h"
#define PROTOPORT 9047 /* default protocol port number */
#define QLEN 6 /* size of request queue */
#define MAX_THREAD 40


int visits = 0; /* counts client connections */

void * socRead (void * args);
void * SendAll(void * args);
void closeAll();
struct sbuf_t{//I used this to pass mutexes and conditions - decided to leave with buffer only
    int 	sda;         /* Buffer array */    
    int 	numCon;     
} sbuf_t;
int exitFlag = 0;
int sd; 
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t usersThrd[MAX_THREAD], sendAllThrd;

char buf[1000]; /* buffer for string the server sends */
int connectionArray[40];
int numCon = 0;
int sd2=-1; /* socket descriptors */
int main(int argc, char const *argv[])
{
	struct hostent *ptrh; /* pointer to a host table entry */
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold server's address */
	struct sockaddr_in cad; /* structure to hold client's address */
	int port; /* protocol port number */
	int alen; /* length of address */
	int n = 0;
	int i=0;
	int pid;
	int thr_err;
	
	userInfo users[MAX_THREAD];
	
	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */
	sad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */
/* Check command-line argument for protocol port and extract */
/* port number if one is specified. Otherwise, use the default */
/* port value given by constant PROTOPORT */
	if (argc > 1) 
	{ /* if argument specified */
		port = atoi(argv[1]); /* convert argument to binary */
	} 
	else 
	{
		port = PROTOPORT; /* use default port number */
	}
	if (port > 0) /* test for illegal value */
		sad.sin_port = htons((u_short)port);
	else 
	{ /* print error message and exit */
		fprintf(stderr,"bad port number %s\n",argv[1]);
		exit(1);
	}

/* Map TCP transport protocol name to protocol number */
	if ( ((int)(ptrp = getprotobyname("tcp"))) == 0) 
	{
		fprintf(stderr, "cannot map \"tcp\" to protocol number");
		exit(1);
	}
/* Create a socket */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) 
	{
		fprintf(stderr, "socket creation failed\n");
		exit(1);
	}
/* Bind a local address to the socket */
	if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) 
	{
		fprintf(stderr,"bind failed\n");
		exit(1);
	}
/* Specify size of request queue */
	if (listen(sd, QLEN) < 0) 
	{
		fprintf(stderr,"listen failed\n");
		exit(1);
	}
/* Main server loop - accept and handle requests */
	if ((thr_err = pthread_create(&sendAllThrd,NULL, SendAll, NULL)) != 0) 
	{
	 	fprintf(stderr, "Err. pthread_create() %s\n", strerror(thr_err));
		exit(EXIT_FAILURE);
	}

	printf("I'm waiting for connections ...\n");
	
	while(!exitFlag)
	{
		if(numCon < 40)
		{
			alen = sizeof(cad);
			fflush(stdout);
			if ( (sd2=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) 
			{
				fprintf(stderr, "accept failed\n");
				exit(1);
			}
			else if(!exitFlag)
			{
				pthread_mutex_lock(&buffer_mutex);
				sbuf_t.sda	= sd2;
				sbuf_t.numCon = numCon;
				//printf("\nbefore creation\n");
					//fflush(stdout);
				if ((thr_err = pthread_create(&usersThrd[numCon],NULL, socRead   , &sbuf_t)) != 0) 
				{
           		 	fprintf(stderr, "Err. pthread_create() %s\n", strerror(thr_err));
            		exit(EXIT_FAILURE);
        		}
				connectionArray[numCon++] = sd2;
								
				pthread_mutex_unlock(&buffer_mutex);

				printf("I received one connection. Now I have %d conections\n", numCon);
			}
		}
	}
	printf("\nKilling threads\n");
	fflush(stdout);
	for (i = 0; i < numCon; ++i)
	{
		pthread_cancel (usersThrd[i]);
	}
	printf("\nKilled!\n");
	fflush(stdout);
	///JOIN here
	closesocket(sd2);
	wait();
	return 0;
}

void * socRead(void * args)
{
	struct sbuf_t targ = *(struct sbuf_t *) args;
	int sd2 = targ.sda;
	int myId = targ.numCon;
	int i;

	int n = 0;
	while (1) 
	{
		memset(buf,'\0',sizeof(buf));
		if(strncmp(buf,"/lg",3))
		{
			//this a login cmd
			//store address
			printf("This is a login\n");

		}
		n = recv(sd2, buf, sizeof(buf), 0);
		if((n>0 && !strcmp(buf,"server exit"))|| exitFlag)
		{
			printf("\nCought exit signal...\n");
			exitFlag = 1;
			closeAll();
			printf("exiting.");
			printf(" .\n");
			fflush(stdout);
			sleep(1);
			pthread_exit((void*)0);
		}
		if(n>0 && !strcmp(buf,"exit"))
		{
			/*kill current connection*/
			printf("Client exiting.");
			pthread_mutex_lock(&buffer_mutex);
			closesocket(sd2);
			for (i = myId; i < numCon-1; ++i)
			{
				connectionArray[i] = connectionArray[i+1];
			}
			numCon--;
			pthread_mutex_unlock(&buffer_mutex);
			printf("Now I have %d conections\n", numCon);
			fflush(stdout);
			sleep(1);
			pthread_exit((void*)0);
		}
		if(n>0)
		{
			printf("\033[22;31mclient: \033[22;37m");
			printf("%s\n",buf);
			fflush(stdout);
			n=0;
		}
		
	}	
}

void * SendAll(void * args)
{
	// struct sbuf_t targ = *(struct sbuf_t *) args;

	char buf2[1000];
	int i;
	while(1)
	{
		memset(buf2,0,sizeof(buf2));	//clear memory
		fgets(buf2,30,stdin);			//read line
		i = strlen(buf2)-1;				//delete CRLF
		if(buf2[i]=='\n') 
			buf2[i] = '\0';
		for (i = 0; i < numCon; ++i)
		{							//send to all clients
			send(connectionArray[i],buf2,strlen(buf2),0);
		}
		if(!strcmp(buf2,"exit")||exitFlag)
		{							//check for exit token
			exitFlag = 1; //let all know that it;s time to exit
			closesocket(sd);
			sleep(2);
			printf("exiting with SendAll\n");
			fflush(stdout);
			exit(0);
		}
	}
}
void closeAll()
{
	int i;
	char buf2[5]={'e','x','i','t'};
	
	for (i = 0; i < numCon; ++i)
	{//close all connections
		printf("sending and closing %d\n" , i);
		fflush(stdout);
		send(connectionArray[i],buf2,strlen(buf2),0);
		closesocket(connectionArray[i]);
		//pthread_cancel (usersThrd[i]);
	}
	closesocket(sd);
	closesocket(sd2);

	printf("Done closeAll\n");
	fflush(stdout);
	exit(0);
}