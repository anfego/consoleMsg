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
void rqNewConnection(int socket);
void deserializer(const char * buf,char * source,char * cmd,char * destination);
void sendError(int socket);
struct sbuf_t{//I used this to pass mutexes and conditions - decided to leave with buffer only
    int 	sda;         /* Buffer array */    
    int 	numCon;     
} sbuf_t;
int exitFlag = 0;
int sd; 
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t usersThrd[MAX_THREAD], sendAllThrd;

int connectionArray[40];
int numCon = 0;
int sd2=-1; /* socket descriptors */
// keeps all user's information
userInfo users[MAX_THREAD];


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
	zeroStatus(users,MAX_USERS);
	
	
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
	char buf[1000]; /* buffer for string the server sends */
	char cmd[4];
	char source[20];
	char msg[140];
	struct sbuf_t targ = *(struct sbuf_t *) args;
	int sd2 = targ.sda;
	int myId = targ.numCon;
	int i;

	int n = 0;
	struct sockaddr_storage sender;
	socklen_t sendsize = sizeof(sender);
	
	while (1) 
	{
		memset(buf,'\0',1000*sizeof(char));

		// bzero(&sender, sizeof(sender));
		//extracts the command from the IN buffer
		n = recv(sd2, buf, sizeof(buf), 0);
		if (n > 0)
		{
			memset(cmd,'\0',4*sizeof(char));
			memset(source,'\0',20*sizeof(char));
			memset(msg,'\0',140*sizeof(char));
			
			deserializer(buf,source,cmd,msg);
			

			printf("CMD: %s\n", cmd );
			printf("Source: %s\n", source );
			printf("MSG: %s\n", msg );
			
			int index = -1;
			int iSource = -1;
			char name[20];
			//this is a login
			if(strncmp(cmd,"/lg",3) == 0)
			{
				memcpy(name,buf+3,strlen(buf+3));
				//this a login cmd
				//store address
				//checks if the user exists
				index = getUserByName(users,name,MAX_USERS);
				if ( index == -1)
				{
					//add user info
					index = findNiceSpot(users,MAX_USERS);
					if( index != -1)
					{
						//index is a free spot - filling it with an information
						memcpy(users[index].name,source,strlen(source)*sizeof(char));
						users[index].status = 1;
						users[index].socketHandler = sd2;
					}
					else
					{
						printf("Server Full: %s \n", source );
						// TODO: it can be extended here to reply with an error msg
						closesocket(sd);
					}	 	

				}
				else
				{
					users[index].status = 1;
				}
				printUserInfo(&users[index]);
			}
			//this is a client logout
			else if(strncmp(cmd,"/ex",3) == 0)
			{
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
			else if(strncmp(cmd,"/nc",3) == 0)
			{
				//this a client requesting new chat
				
				printf("Finding %s\n", msg );
				printAllListInfo(users,MAX_USERS);
				// index = isUserConnected(users,msg,MAX_USERS);
				if(index >= 0)
				{
					//accepted
					printf("Request connection to %s\n",users[index].name );
					rqNewConnection(users[index].socketHandler);
				}
				else
				{

					printf("The User is not Connected\n");
					iSource = getUserByName(users,source,MAX_USERS);
					sendError(users[iSource].socketHandler);
				}
			}


			
		// #endregion
			
			// printf("\033[22;31mclient: \033[22;37m");
			printf("EndOfOneWhileCycle%s\n",buf);
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
		if(!strcmp(buf2,"exit")|| exitFlag )
		{							//check for exit token
			for (i = 0; i < MAX_USERS; ++i)
			{							//send to all clients
				// send(connectionArray[i],buf2,strlen(buf2),0);
				if(users[i].status == 1)
					send(users[i].socketHandler,"/ex",3*sizeof(char),0);
			}
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
void rqNewConnection(int socket)
{
	char buf2[140] = "/rq";
	send(socket,buf2,strlen(buf2),0);

}
void sendError(int socket)
{
	char buf2[10];
	memset(buf2,'\0',10*sizeof(char));
	memcpy(buf2,"/er",3*sizeof(char));
	send(socket,buf2,strlen(buf2),0);

}
void deserializer(const char * buf,char * source,char * cmd,char * msg)
{
	int i;
	char localBuf[1000];
	memcpy(localBuf,buf,strlen(buf)*sizeof(char));
	
	memcpy(cmd,localBuf,3*sizeof(char));
	
	// char * ptr;
	
	// ptr = strtok(localBuf+3,"#");
	
	// memcpy(source,ptr,strlen(ptr)*sizeof(char));
	
	// ptr = strtok(NULL,"#");
	// memcpy(msg,ptr,strlen(ptr)*sizeof(char));
	
	

	for(i=3;i<strlen(localBuf);++i)
	{
		printf("%d %s\n", i,localBuf+i);	
		if(strncmp(localBuf+i,"#",1) == 0)
		{
			break;
		}

	}
	if(i == strlen(localBuf))
	{
		printf("ERROR!\n");
		
		
	}
	else
	{

	}
		memcpy(source,localBuf+3,(i-3)*sizeof(char));
		memcpy(msg,localBuf+i+1,strlen(localBuf)*sizeof(char));

}

