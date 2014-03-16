/**************************************************************************************
 FILE NAME:			client.c
 PROGRAMMER:		Ivan Syzonenko
 					Andres F. Gomez
 CLASS:				CSCI 6300
 DUE DATE:			Spring 2014
 INSTRUCTOR:		Dr. Gu
 DESCRIPTION:		Allocates a socket. Waits and prints messages from
					server. Sends messages to server
					
					Syntax: client [ host [port] ]

					host - name of a computer on which server is executing
					port - protocol port number server is using
					
					Note: Both arguments are optional. If no host name is specified,
					the client uses "localhost"; if no protocol port is
					specified, the client uses the default given by PROTOPORT.
**************************************************************************************/
#ifndef unix
#define WIN32
#include <windows.h>
#include <winsock.h>
#include <io.h>
#else
#define closesocket close
#include <sys/types.h>
#include <sys/socket.h>

#include <ifaddrs.h>	
				
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#endif
#include <stdio.h>
#include <string.h>
#define PROTOPORT 9047 /* default protocol port number */
//extern int errno;
char localhost[] = "localhost"; /* default host name */
// pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t  sendAllThrd;// usersThrd[MAX_THREAD],
int newConnection = 0;//new connection flag: 1 - if a new connection is to be established;
int CurrConnIDX = 0; // counts latest element in pthread array
int error = 0;//use for phtread error during creation

// Helper Funtions

int ConsoleEngine();
//this retunrs a socket descriptor
int createSocket(char* host);

int openConnection();
int login (int socketHandler);

int main(int argc, char *argv[])
//int argc;
//char *argv[];
{
	int socketHandler = -1;
	if( (socketHandler = createSocket(argv[1])) < 0 )
		return 1;
	if ( !login(socketHandler) )
		return 1;
	
	if (socketHandler > 0)
		closesocket(socketHandler);


	// create control pthread that will signal to all other threds to exit
	// if ((error = pthread_create(&sendAllThrd,NULL, signalThreadfunc, NULL)) != 0) 
	// {
	//  	fprintf(stderr, "Err. pthread_create() %s\n", strerror(error));
	// 	exit(EXIT_FAILURE);
	// }
	// // create pthread for particular connection
	// while(1)
	// {
	// 	if(newConnection)
	// 	{
	// 		pthreadArray[CurrConnIDX++] = pthread_create......
	// 	}
	return 0;
}
int createSocket(char* host)
{
	struct hostent *ptrh; /* pointer to a host table entry */
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold an IP address */
	int sd; /* socket descriptor */
	int port = PROTOPORT; /* protocol port number */
	
	int n; /* number of characters read */
	int pid;
	char buf[1000], buf2[1000]; /* buffer for data from the server */
	char sent = 0;
	int i;
	int error = 0;
	// clear sockaddr structure
	memset((char *)&sad,0,sizeof(sad)); 
	
	sad.sin_family = AF_INET; /* set family to Internet */
	/* Check command-line argument for protocol port and extract */
	/* port number if one is specified. Otherwise, use the default */
	/* port value given by constant PROTOPORT */

	 /* test for legal value */
	sad.sin_port = htons((u_short)port);
	
	/* Check host argument and assign host name. */

	/* Convert host name to equivalent IP address and copy to sad. */
	ptrh = gethostbyname(host);

	if ( ((char *)ptrh) == NULL ) 
	{
		fprintf(stderr,"invalid host: %s\n", host);
		return -1;
	}
	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);
	/* Map TCP transport protocol name to protocol number. */
	if ( ((int)(ptrp = getprotobyname("tcp"))) == 0) 
	{
		fprintf(stderr, "cannot map \"tcp\" to protocol number");
		return -1;
	}
	/* Create a socket. */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);

	if (sd < 0) 
	{
		fprintf(stderr, "socket creation failed\n");
		return -1;
	}
	/* Connect the socket to the specified server. */
	if (connect(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) 
	{
		fprintf(stderr,"connect failed\n");
		return -1;
	}
	return sd;
}
int login(int socketHandler)
{

	char buf2[1000];
	
	struct ifaddrs *addrs, *tmp;
	getifaddrs(&addrs);
	int interfacesNum = 0;
	tmp = addrs;

	while (tmp) 
	{

	    if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
	    {
	        struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
	        if(interfacesNum == 1)
	        {
	        	memset(buf2,'\0',sizeof(buf2));	//clear memory
        		memcpy(buf2,"/lg",sizeof(char)*3);
        		memcpy(buf2+3,inet_ntoa(pAddr->sin_addr),strlen(inet_ntoa(pAddr->sin_addr)));			//read line	
        		
        		
        		

	        	
	        }
	        interfacesNum++;
	    }
		tmp = tmp->ifa_next;

	}

	freeifaddrs(addrs);
	send(socketHandler,buf2,strlen(buf2),0);

	return 0;
}
