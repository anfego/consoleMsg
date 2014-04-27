/*************************************************************************************
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
************************************************************************************/
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
char localhost[] = "raspberrypi"; /* default host name */
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
int createSocket(char* host, int port);

int openConnection(int socket);
void closeConnection(int socket);
int login (int socketHandler);
void getMyIp(char * ip, int socket);
int sendMsg(char * msg, int socket, char * cmd);
void deserializer(const char * buf,char * source,char * cmd,char * msg);
void deserializer2(const char * buf,char * source,char * msg);
void * chat(void * args);

int main(int argc, char *argv[])
{
	//Zero all connection Status
	char buf[1000];
	zeroStatus(users,MAX_USERS);

	if( (users[0].socketHandler = createSocket(argv[1], PROTOPORT)) < 0 )
		return 1;

	users[0].status = 1;

	login(users[0].socketHandler);
	printf("Login\n");
	// users[0].name = "_server_main_";
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
int createSocket(char* host, int portIn)
{
	struct hostent *ptrh; /* pointer to a host table entry */
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold an IP address */
	int sd; /* socket descriptor */
	// int port = PROTOPORT; /* protocol port number */
	int port = portIn;

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
	printf("Host %s of LENT %d port %d\n",host, strlen(host), portIn);
	if ( ((char *)ptrh) == NULL ) 
	{
		fprintf(stderr,"invalid host: %s\n", host);
		return -1;
	}
	
	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);
	/* Map TCP transport protocol name to protocol number. */
	printf("*\n");
	if ( ((int)(ptrp = getprotobyname("tcp"))) == 0) 
	{
		fprintf(stderr, "cannot map \"tcp\" to protocol number");
		return -1;
	}
	
	/* Create a socket. */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	printf("*\n");
	if (sd < 0) 
	{
		fprintf(stderr, "socket creation failed\n");
		return -1;
	}
	
	/* Connect the socket to the specified server. */
	printf("*\n");
	if (connect(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) 
	{
		fprintf(stderr,"connect failed\n");
		return -1;
	}
	printf("5\n");
	return sd;
}
int login(int socketHandler)
{

	char buf2[500];
	sendMsg("%",socketHandler,"/lg");
	
	// struct ifaddrs *addrs, *tmp;
	// getifaddrs(&addrs);
	// int interfacesNum = 0;
	// tmp = addrs;

	// while (tmp) 
	// {

	//     if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
	//     {
	//         struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
	//         if(interfacesNum == 1)
	//         {
	//         	memset(buf2,'\0',sizeof(buf2));	//clear memory
 	//        		memcpy(buf2,"/lg",sizeof(char)*3);
 	//        		memcpy(buf2+3,inet_ntoa(pAddr->sin_addr),strlen(inet_ntoa(pAddr->sin_addr)));			//read line	
 	       		
 	//        	}
	//         interfacesNum++;
	//     }
	// 	tmp = tmp->ifa_next;

	// }

	// freeifaddrs(addrs);
	// send(socketHandler,buf2,strlen(buf2),0);

	return 0;
}

void closeConnection(int socket)
{
	char buf2[10];
	printf("Client Exiting\n");
	sendMsg("%",socket,"/ex");

	// memset(buf2,'\0',sizeof(buf2));	//clear memory
	// memcpy(buf2,"/ex",sizeof(char)*3);
	// send(socket,buf2,strlen(buf2),0);
	if (socket > 0)
		closesocket(socket);
}

int sendMsg(char * msg, int socket,char * cmd)
{
	char myIp[140];
	char bufOut[200];
	
	memset(bufOut,'\0',200*sizeof(char));
	
	getMyIp(myIp,socket);
	
	printf("myIp: %s\n", myIp);
	printf("mySocket: %d\n", socket);
	
	memcpy(bufOut,cmd,3*sizeof(char));
	printf("bufOut CMD %s\n", bufOut );
	
	memcpy(bufOut+strlen(bufOut),myIp,strlen(myIp)*sizeof(char));
	printf("bufOut IP %s\n", bufOut );

	memcpy(bufOut+strlen(bufOut),"#",1*sizeof(char));
	printf("bufOut # %s\n", bufOut );
	
	memcpy(bufOut+strlen(bufOut),msg,strlen(msg)*sizeof(char));
	
	printf("bufOut LENT %d\n", strlen(bufOut) );
	send(socket,bufOut,strlen(bufOut),0);
	
	printf("\t%d:bufOut Total %s\n",socket, bufOut );
	return 0;
}
void consoleEngine()
{
	char buf[140];
	char destination[140];
	memset(buf, '\0', 140*sizeof(char));
	int n, index;

	char buf2[1000];
	memset(buf2,'\0',sizeof(buf2));	//clear memory
	// memcpy(buf2,"/nc192.168.4.132",sizeof(char)*16);
	while(1)	
	{
		memset(buf2,'\0',1000*sizeof(char));	//clear memory
		memset(destination,'\0',140*sizeof(char));	//clear memory
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
			memcpy(destination,buf2+3,(strlen(buf2)-3)*sizeof(char));
			sendMsg(destination,users[0].socketHandler,"/nc");

		}
		else if(strncmp(buf2,"/cn",3) == 0)
		{
			int index = -1;
			char cmd[4];
			char msg[140];
			memset(msg, '\0', 140*sizeof(char));
			memset(cmd, '\0', 4*sizeof(char));
			sscanf(buf2,"%s %d %s",cmd,&index,msg);
			// memcpy(destination,buf2+3,1*sizeof(char));
			// memcpy(msg,buf2+3,(2)*sizeof(char));
			// index = atoi(destination);
			printf("Sending %s to index:%d and cmd %s\n", msg, index,cmd);
			
			sendMsg(msg,users[index].socketHandler,cmd);
		}
		else if(strncmp(buf2,"/us",3) == 0)
		{
			printAllListInfo(users,MAX_USERS);
		}
		
		// 	printf("\033[22;31mserver: \033[22;37m");
			printf("%s\n",buf);
			fflush(stdout);
			memset(buf,0,sizeof(buf));
		
	}

}
void * chat (void * chatInfo)
{
	userInfo * chat = (userInfo *)chatInfo;
	int n = 0;
	char buf[140];
	char cmd[4];
	char source[20];
	char msg[140];
	if (amIClient(chat) == CLIENT)
	{
		chat->socketHandler = createSocket(chat->name, chat->port);
		printf("Inside pthread for socket: %d as client on port %d\n", chat->socketHandler,chat->port);
	// if (connect(users[index].socketHandler, (struct sockaddr *)&sad, sizeof(sad)) < 0) 
			// {
			// 	fprintf(stderr,"connect failed\n");

			// }
			


	}
	else
	{

		printf("Inside pthread for socket: %d as server\n", chat->socketHandler);
		int newSocket = Accept_Connection(chat->socketHandler);
		closesocket(chat->socketHandler);
		chat->socketHandler = newSocket;
		printf("connection Acepted\n");
	}
	
	while(1)
	{
		
		memset(buf, '\0', 140*sizeof(char));
		n = recv(chat->socketHandler, buf, 140*sizeof(char), 0);
		if (n < 0)
		{
			// printf("Error in socket - %d\n",sock_errno());\
			Error_Check(-1,"Reading socket");
			
  			exit(0);
		}
		// printf("\t\t\t\tBytes%d\n",n );
		if(n > 0)
		{
		
			memset(cmd,'\0',4*sizeof(char));
			memset(source,'\0',20*sizeof(char));
			memset(msg,'\0',140*sizeof(char));
			
			deserializer(buf,source,cmd,msg);
			
			printf("CMD: %s\n", cmd );
			printf("Source: %s\n", source );
			printf("MSG: %s\n", msg );

			if(strncmp(cmd,"/me",3) == 0)
			{
			//got encrypted message	

			}
			else if(strncmp(cmd,"/cn",3) == 0)
			{
				printf("received: %s\n", msg);
			}
			else if(strncmp(cmd,"/ex",3) == 0)
			{
				// /closeConnection(chat->socketHandler);
				closesocket(chat->socketHandler);
				break;
			}
		}
	}
}
void * clientEngine(void * socketIn)
{
	int socket = *((int *)socketIn);
	

	// fd_set active_fd_set, read_fd_set, write_fd_set;
	// FD_ZERO (&active_fd_set);
	// FD_SET (socket, &active_fd_set);
	// read_fd_set = active_fd_set;

	struct sockaddr_in clientname;
	struct timeval tv;
	int retVal,i;
	/* Wait up to five seconds. */
	tv.tv_sec = 0;
	tv.tv_usec = 500;


	char buf[140];
	char buf2[1000];
	
		
	char cmd[4];
	char source[20];
	char msg[140];
	
	int n, index;
	struct sockaddr_in sad; /* structure to hold an IP address */
	printf("LISTENING on %d!!!!!\n",socket);	
	while(1)
	{
		// retVal = select (FD_SETSIZE, &read_fd_set, NULL, NULL, &tv);
		// if (retVal == -1)
		// {
		// 	perror ("select");
		// 	exit (EXIT_FAILURE);
		// }
		// for (i = 0; i < FD_SETSIZE; ++i)
		// {

		// 	if (FD_ISSET (i, &read_fd_set))
		// 	{
		// 		printf("\t\tSelect %d socket\n",retVal,i );

		// 	}
		// }
		memset(buf, '\0', 140*sizeof(char));
		memset(cmd, '\0', 4*sizeof(char));

		n = recv(socket, buf, sizeof(buf), 0);
		
		if(n > 0)
		{
		
			memset(cmd,'\0',4*sizeof(char));
			memset(source,'\0',20*sizeof(char));
			memset(msg,'\0',140*sizeof(char));
			
			deserializer(buf,source,cmd,msg);
			
			printf("CMD: %s\n", cmd );
			printf("Source: %s\n", source );
			printf("MSG: %s\n", msg );

			if(strncmp(cmd,"/ex",3) == 0)
			{
				/*kill current connection*/
				printf("exiting.");
				closesocket(socket);
				printf(" .\n");
				fflush(stdout);
				break;

			}
			if(strncmp(cmd,"/rq",3) == 0)
			{
				// check wheter a new connection is allowed
				if(getConnectedUsers(users,MAX_USERS)< MAX_USERS)
				{
					//connection can be stablished
					// create control pthread that will signal to all other threds to exit
					int port = 0;
					index = findNiceSpot(users,MAX_USERS);
					// memcpy(buf2,buf+3,strlen(buf+3));

					users[index].socketHandler = New_Socket(&port);
					users[index].status = 1;
					memcpy(users[index].name, msg, strlen(msg)*sizeof(char));
					
					printUserInfo(&users[index]);
						
					printf("\tThis is the new port: %d for %s\n", port,msg);
					
					memcpy(buf2,msg,strlen(msg)*sizeof(char));
					memcpy(buf2+strlen(buf2),"#",sizeof(char));
					
					sprintf(buf2+strlen(buf2),"%d",port);

					
					sendMsg(buf2,users[0].socketHandler,"/so");
					
					printf("Before connection\n" );

					setRole(users,index,SERVER);
					
					// listenConnection(users[index].socketHandler);

					// Accept_Connection(users[index].socketHandler);
					
					// openConnection(users[index].socketHandler);

					printf("After socket creation socket= %d\n", users[index].socketHandler );
					
					if ((error = pthread_create(
										&(users[index].userPThread),
										NULL, 
										chat,
										(void *)&(users[index])))
								!= 0) 
					{
					 	fprintf(stderr, "Err. pthread_create() %s\n", strerror(error));
						exit(EXIT_FAILURE);
					}
					
					// notify the server about the new socket to handle the new connection

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
					// closesocket(users[index].socketHandler);
					break;

				}

			}
			else if(strncmp(cmd,"/er",3) == 0)
			{
				printf("There was an error, commands not executed!\n");
			}

			else if(strncmp(cmd,"/so",3) == 0)
			{

				int port;
				printf("Ok stablish connection to %s\n",msg);
				// extract information from msg
				deserializer2(msg,source,msg);
				index = findNiceSpot(users,MAX_USERS);

				printf("Connecting to : %s -> %d\n", source, strlen(source) );
				port = atoi(msg);
				printf("BY port : %d\n", port );

				memcpy(users[index].name, source, strlen(source)*sizeof(char));
				sleep(2);
				printf("Before socket Create\n" );

				setPort(users,index,port);
				setRole(users,index,CLIENT);

				// users[index].socketHandler = New_Socket(&port);
				// users[index].socketHandler = createSocket(source, port);

				// if (connect(users[index].socketHandler, (struct sockaddr *)&sad, sizeof(sad)) < 0) 
				// {
				// 	fprintf(stderr,"connect failed\n");

				// }
				
				// users[index].socketHandler = createSocket(users[index].name);

				printf("socketHandler : %d\n", users[index].socketHandler);


				printUserInfo(&users[index]);

				
				if ((error = pthread_create(
										&(users[index].userPThread),
										NULL,
										chat,
										(void*)&(users[index]))) != 0) 
				{
				 	fprintf(stderr, "Err. pthread_create() %s\n", strerror(error));
					exit(EXIT_FAILURE);
				}


			}
			else if(strncmp(cmd,"/cn",3) == 0)
			{
				printf("received: %s\n", msg);
			}

	
		}
	}
}

void getMyIp(char * ip, int socket)
{

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
		memset(ip,'\0',140*sizeof(char));	//clear memory
		memcpy(ip,inet_ntoa(pAddr->sin_addr),strlen(inet_ntoa(pAddr->sin_addr)));			//read line	

	}
	interfacesNum++;
	}
	tmp = tmp->ifa_next;

	}


}

void deserializer(const char * buf,char * source,char * cmd,char * msg)
{
	int i;
	char localBuf[1000];
	memset(localBuf, '\0', 1000*sizeof(char));

	memcpy(localBuf,buf,strlen(buf)*sizeof(char));
	
	memcpy(cmd,localBuf,3*sizeof(char));

	for(i=3;i<strlen(localBuf);++i)
	{
		// printf("%d %s\n", i,localBuf+i);	
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
		memcpy(source,localBuf+3,(i-3)*sizeof(char));
		memcpy(msg,localBuf+i+1,strlen(localBuf)*sizeof(char));

	}

}

void deserializer2(const char * buf,char * source,char * msg)
{
	int i;
	char localBuf[1000];
	memset(localBuf, '\0', 1000*sizeof(char));
	memcpy(localBuf,buf,strlen(buf)*sizeof(char));

	for(i=0;i<strlen(localBuf);++i)
	{
		
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

		memcpy(msg,localBuf+i+1,strlen(localBuf+i+1)*sizeof(char));
		memset(msg+strlen(localBuf+i+1), '\0', (1000-strlen(localBuf+i+1))*sizeof(char) );
		memcpy(source,localBuf,i*sizeof(char));

	}


}
int listenConnection(int socketHandler)
{
	int resp=0;
	resp = listen(socketHandler, 6);
	if (resp < 0) 
	{
		fprintf(stderr,"listen failed\n");
		exit(1);
	}
	if(resp == EADDRINUSE)
	{
		printf("Already in use\n");
	}

	return 0;
}
int openConnection(int socket)
{
	int alen;
	struct sockaddr_in cad; /* structure to hold client's address */
	/* Bind a local address to the socket */
	
/* Specify size of request queue */
	if (listen(socket, 6) < 0) 
	{
		fprintf(stderr,"listen failed\n");
		exit(1);
	}
/* Main server loop - accept and handle requests */
	
	printf("I'm waiting for connections ...\n");
	
	while(1)
	{
		alen = sizeof(cad);
		fflush(stdout);
		if ( (socket=accept(socket, (struct sockaddr *)&cad, &alen)) < 0) 
		{
			fprintf(stderr, "accept failed\n");
			exit(1);
		}
		else 
		{
			// if ((thr_err = pthread_create(&usersThrd[numCon],NULL, socRead   , &sbuf_t)) != 0) 
			// {
   //     		 	fprintf(stderr, "Err. pthread_create() %s\n", strerror(thr_err));
   //      		exit(EXIT_FAILURE);
   //  		}
			printf("I received one connection. Now I have %d conections\n", 1);
			break;
		}

	}
	return socket;

}