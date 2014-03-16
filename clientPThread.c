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
//Project Libraries
#include "userInfo.h"
#include "communications.h"



#define PROTOPORT 9047 /* default protocol port number */
//extern int errno;
char localhost[] = "localhost"; /* default host name */
// pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t  sendAllThrd;// usersThrd[MAX_THREAD],
int newConnection = 0;//new connection flag: 1 - if a new connection is to be established;
int CurrConnIDX = 0; // counts latest element in pthread array
int error = 0;//use for phtread error during creation


pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t usersThrd[MAX_USERS], sendAllThrd;


userInfo users[MAX_USERS]; //store connections

// Helper Funtions

void consoleEngine();
void * clientEngine(void * socketIn);
//this retunrs a socket descriptor
int createSocket(char* host);

int openConnection();
void closeConnection(int socket);
int login (int socketHandler);
int sendMsg(char * msg, int socket);
void * chat(void * args);

int main(int argc, char *argv[])
{
	//Zero all connection Status
	
	zeroStatus(users,MAX_USERS);

	if( (users[0].socketHandler = createSocket(argv[1])) < 0 )
		return 1;

	users[0].status = 1;
	// users[0].name = "_server_main_";

	login(users[0].socketHandler);
	// Create a Pthread to handle commands
	// create control pthread that will signal to all other threds to exit
	if ((error = pthread_create(&(users[0].userPThread),NULL, clientEngine, (void *)&(users[0].socketHandler) )) != 0) 
	{
	 	fprintf(stderr, "Err. pthread_create() %s\n", strerror(error));
		exit(EXIT_FAILURE);
	}

	consoleEngine();

	
	
	
	printf("\tBye\n");



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

void closeConnection(int socket)
{
	char buf2[10];
	printf("Client Exiting\n");
	memset(buf2,'\0',sizeof(buf2));	//clear memory
	memcpy(buf2,"/ex",sizeof(char)*3);
	send(socket,buf2,strlen(buf2),0);
	if (socket > 0)
		closesocket(socket);
}

int sendMsg(char * msg, int socket)
{
	return 0;
}
void consoleEngine()
{
	char buf[140];
	memset(buf, '\0', 140*sizeof(char));
	int n, index;

	char buf2[1000];
	memset(buf2,'\0',sizeof(buf2));	//clear memory
	// memcpy(buf2,"/nc192.168.4.132",sizeof(char)*16);
	while(1)	
	{
		memset(buf2,0,sizeof(buf2));	//clear memory
		fgets(buf2,30,stdin);			//read line
		int i = strlen(buf2)-1;				//delete CRLF
		if(buf2[i]=='\n') 
			buf2[i] = '\0';
	
		if(!strcmp(buf2,"/ex"))
		{							//check for exit token
			for (i = MAX_USERS-1; i >= 0; --i)
			{							//send to all clients
				// send(connectionArray[i],buf2,strlen(buf2),0);
				if(users[i].status == 1)
				{
					pthread_cancel(users[i].userPThread);
					closeConnection(users[i].socketHandler);
				}

				
			}

			fflush(stdout);
			exit(0);
		}
		else if (strncmp(buf2,"/nc",3) == 0)
		{
			int index = -1;
			send(users[0].socketHandler, buf2,strlen(buf2),0);
			// index = findNiceSpot
			// pthread_create(users.);
			//TODO: Create a Pthread to handle new connection
		}
		
		// 	printf("\033[22;31mserver: \033[22;37m");
			printf("%s\n",buf);
			fflush(stdout);
			memset(buf,0,sizeof(buf));
		
	}

}
void * chat (void * args)
{


}
void * clientEngine(void * socketIn)
{
	int socket = *((int *)socketIn);
	char buf[140];
	char cmd[4];
	int n, index;
	char buf2[1000];
	printf("LISTENING on %d!!!!!\n",socket);	
	while(1)
	{
		memset(buf, '\0', 140*sizeof(char));
		memset(cmd, '\0', 4*sizeof(char));
		n = recv(socket, buf, 140*sizeof(char), 0);
		memcpy(cmd,buf,3*sizeof(char));
		if(n >0)
		{
			printf("\tCMD: %s\n",cmd);
			if(strncmp(buf,"/ex",3) == 0)
			{
				/*kill current connection*/
				printf("exiting.");
				closesocket(socket);
				printf(" .\n");
				fflush(stdout);
				break;

			}
			if(strncmp(buf,"/rq",3) == 0)
			{
				if(getConnectedUsers(users,MAX_USERS)< MAX_USERS)
				{
					//connection can be stablished
						// create control pthread that will signal to all other threds to exit
					int port = 0;
					index = findNiceSpot(users,MAX_USERS);

					users[index].socketHandler = New_Socket(&port);

					printf("\tThis is the new socket: %d\n", users[0].socketHandler);
					printf("\tThis is the new socket: %d\n", users[index].socketHandler);

					// if ((error = pthread_create(&(users[index].userPThread),NULL, chat, NULL)) != 0) 
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
					closesocket(users[index].socketHandler);
					break;

				}

			}
			else if(strncmp(buf,"/er",3) == 0)
			{
				printf("There was an error, commands not executed!\n");
			}
	
		}
	}
}