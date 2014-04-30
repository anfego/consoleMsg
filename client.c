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
#include <time.h>		
#include <assert.h>
//Project Libraries
#include "userInfo.h"
#include "communications.h"
#include "bigd.h"
#include "bigdRand.h"


#define PROTOPORT 9047 /* default protocol port number */
#define KEYSIZE 1024					
#define BUF_SIZE 300					
//extern int errno;
char localhost[] = "raspberrypi"; /* default host name */
// pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_t  sendAllThrd;// usersThrd[MAX_THREAD],
int newConnection = 0;//new connection flag: 1 - if a new connection is to be established;
int CurrConnIDX = 0; // counts latest element in pthread array
int error = 0;//use for phtread error during creation
/*******************************************************/
// Encryption vars
BIGD n, e, d;
/*******************************************************/


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

int generateRSAKey(BIGD n, BIGD e, BIGD d);
void Decryptor(unsigned char *inMsg, unsigned char *outMsg, BIGD d, BIGD n);
void encrypt(unsigned char *inMsg, BIGD n, BIGD e, unsigned char *outMsg);
void setDN(userInfo * user, unsigned char *msg, int i);

int main(int argc, char *argv[])
{
	//Zero all connection Status
	char buf[1000];
	zeroStatus(users,MAX_USERS);

	if( (users[0].socketHandler = createSocket(argv[1], PROTOPORT)) < 0 )
		return 1;

	users[0].status = 1;
	sscanf(argv[1],"%s",users[0].name);
	n = bdNew();
	e = bdNew();
	d = bdNew();
	generateRSAKey(n, e, d);
	
	

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

	
	
	bdFree(&n);
	bdFree(&e);
	bdFree(&d);
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

	char buf2[2*BUF_SIZE+30];
	int size = 0;
	memset(buf2, '\0', (2*BUF_SIZE+30)*sizeof(char));
	bdConvToHex(d, buf2, BUF_SIZE);
	size = strlen(buf2);
	buf2[size] = '@';
	bdConvToHex(n, buf2+size+1, BUF_SIZE);
	// printf("KEYS OUT\n\t\t\t%s\n",buf2);
	sendMsg(buf2,socketHandler,"/lg");
	
	return 0;
}

void closeConnection(int socket)
{
	char buf2[10];
	printf("Client Exiting\n");
	sendMsg("%",socket,"/ex");
	if (socket > 0)
		closesocket(socket);
}

int sendMsg(char * msg, int socket,char * cmd)
{
	char myIp[140];
	char bufOut[1000];
	
	memset(bufOut,'\0',1000*sizeof(char));
	
	getMyIp(myIp,socket);
	
	
	
	
	memcpy(bufOut,cmd,3*sizeof(char));
	
	
	memcpy(bufOut+strlen(bufOut),myIp,strlen(myIp)*sizeof(char));
	

	memcpy(bufOut+strlen(bufOut),"#",1*sizeof(char));
	
	
	memcpy(bufOut+strlen(bufOut),msg,strlen(msg)*sizeof(char));
	
	
	send(socket,bufOut,strlen(bufOut),0);
	
	// printf("\t%d:bufOut Total %s\n",socket, bufOut );
	return 0;
}
void consoleEngine()
{
	BIGD oe, on;
	char buf[140];
	char destination[140];
	memset(buf, '\0', 140*sizeof(char));
	int index;

	char buf2[1500];
	memset(buf2,'\0',sizeof(buf2));	//clear memory
	char cmd[4];
	char msg[1400];
	// memcpy(buf2,"/nc192.168.4.132",sizeof(char)*16);
	
	while(1)	
	{
		fgets(buf2,1500,stdin);						//read line
		memset(destination,'\0',140*sizeof(char));	//clear memory
		memset(msg, '\0', 1400*sizeof(char));
		memset(cmd, '\0', 4*sizeof(char));
		int i = strlen(buf2)-1;						//delete CRLF
		sscanf(buf2,"%s",cmd);
		if(buf2[i]=='\n') 
			buf2[i] = '\0';
	
		if(!strcmp(cmd,"/ex"))
		{							//check for exit token
			for (i = MAX_USERS-1; i >= 0; --i)
			{							//send to all clients
				if(users[i].status == 1)
				{
					pthread_cancel(users[i].userPThread);
					closeConnection(users[i].socketHandler);
				}
			}
			exit(0);
		}
		else if (strncmp(cmd,"/nc",3) == 0)
		{
			sscanf(buf2,"%s %s",cmd,destination);
			sendMsg(destination,users[0].socketHandler,cmd);

		}
		else if(strncmp(cmd,"/cn",3) == 0)
		{
			int index = -1;
			sscanf(buf2,"%s %d %99[a-zA-Z0-9 ]s",cmd,&index,msg);

			sendMsg(msg,users[index].socketHandler,cmd);
		}
		else if(strncmp(buf2,"/us",3) == 0)
		{
			printAllListInfo(users,MAX_USERS);
		}
		else if(strncmp(buf2,"/me",3) == 0)
		{
			int index = -1;
			oe = bdNew();
			on = bdNew();
			
			sscanf(buf2,"%s %d %99[a-zA-Z0-9 ]s",cmd,&index,msg);
			printf("N KEY is :%s\n", users[index].n);
			printf("D KEY is :%s\n", users[index].d);

			bdConvFromHex(on, users[index].n);
			bdConvFromHex(oe, users[index].d);
			printf("GOING TO ENCRYPT\n");
			encrypt(msg, on, oe, msg);
			printf("DONE ! GOING TO DONE\n");
			sendMsg(msg,users[index].socketHandler,cmd);
			bdFree(&oe);
			bdFree(&on);
			
		}

		memset(buf2,'\0',1000*sizeof(char));		//clear memory
		fflush(stdin);
		
	
	}

}
void * chat (void * chatInfo)
{
	BIGD oe, on;
	int chat = *(int *)chatInfo;
	int received = 0;
	char buf[1400];
	char cmd[4];
	char source[20];
	char msg[BUF_SIZE];
	setStatus(users,chat);
	int index = -1;
	if (amIClient((users+chat)) == CLIENT)
	{
		(users+chat)->socketHandler = createSocket((users+chat)->name, (users+chat)->port);
	}
	else
	{
		int newSocket = Accept_Connection((users+chat)->socketHandler);
		closesocket((users+chat)->socketHandler);
		(users+chat)->socketHandler = newSocket;
	}
	while(1)
	{
		
		memset(buf, '\0', 1400*sizeof(char));
		received = recv((users+chat)->socketHandler, buf, 1400*sizeof(char), 0);
		if (received < 0)
		{
	
			Error_Check(-1,"Reading socket");
			exit(0);
		}
		if(received > 0)
		{
			fflush(stdout);
			memset(cmd,'\0',4*sizeof(char));
			memset(source,'\0',20*sizeof(char));
			memset(msg,'\0',BUF_SIZE*sizeof(char));
			
			deserializer(buf,source,cmd,msg);
			
			printf("CMD: %s\n", cmd );
			printf("Source: %s\n", source );
			printf("MSG: %s\n", msg );

			if(strncmp(cmd,"/me",3) == 0)
			{
			// 	//Here both sides recieve here
			// 	//each side have to use the chat key
			// 	oe = bdNew();
			// 	on = bdNew();
			// 	bdConvFromHex(on, users[chat].n);
			// 	bdConvFromHex(oe, users[chat].d);
			// 	Decryptor(msg, msg, on, oe);
			// 	bdFree(&oe);
			// 	bdFree(&on);
			// //got encrypted message	
			// 	printf("received: %s\n", msg);


				//Here both sides recieve here
				//each side have to use the chat key
				// oe = bdNew();
				// on = bdNew();
				// bdConvFromHex(on, users[chat].n);
				// bdConvFromHex(oe, users[chat].d);

				// Decryptor(msg, msg, n, e);
				// bdFree(&oe);
				// bdFree(&on);
				//got encrypted message	
// =======
				Decryptor(msg, msg, e, n);
				// bdFree(&oe);
				// bdFree(&on);
			//got encrypted message	
// >>>>>>> 4f6a033c2e783386f1679b2f9e8523787712a8c5
				printf("received: %s\n", msg);


			}
			else if(strncmp(cmd,"/cn",3) == 0)
			{
				printf("received: %s\n", msg);
			}
			else if(strncmp(cmd,"/ex",3) == 0)
			{
				closesocket((users+chat)->socketHandler);
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


	char buf[1400];
	char buf2[1000];
	
		
	char cmd[4];
	char source[20];
	char msg[1400];
	
	int received, index, i;
	struct sockaddr_in sad; /* structure to hold an IP address */
	
	while(1)
	{

		memset(buf, '\0', 1400*sizeof(char));
		memset(cmd, '\0', 4*sizeof(char));

		received = recv(socket, buf, sizeof(buf), 0);

		
		if(received > 0)
		{
		
			memset(cmd,'\0',4*sizeof(char));
			memset(source,'\0',20*sizeof(char));
			memset(msg,'\0',1400*sizeof(char));
			
			deserializer(buf,source,cmd,msg);
			
			printf("CMD: %s\n", cmd );
			printf("Source: %s\n", source );
			printf("MSG: %s\n", msg );

			if(strncmp(cmd,"/ex",3) == 0)
			{
				/*kill current connection*/
				printf("exiting.");
				for (i = MAX_USERS-1; i >= 0; --i)
				{							//send to all clients
					if(users[i].status == 1)
					{
						pthread_cancel(users[i].userPThread);
						closeConnection(users[i].socketHandler);
					}
				}
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
					users[index].socketHandler = New_Socket(&port);
					
					setStatus(users,index);
					sscanf(msg,"%s %s %s",users[index].name, users[index].d, users[index].n);

					// memcpy(users[index].name, msg, strlen(msg)*sizeof(char));
					// printf("KEY received for %d:\t%s\n", index, users[index].d);
					
						
					
					memcpy(buf2,users[index].name,strlen(users[index].name)*sizeof(char));
					memcpy(buf2+strlen(buf2),"#",sizeof(char));
					
					sprintf(buf2+strlen(buf2),"%d",port);

					sendMsg(buf2,users[0].socketHandler,"/so");
					
					
					setRole(users,index,SERVER);
					
					if ((error = pthread_create(
										&(users[index].userPThread),
										NULL,
										chat,
										(void*)&(index))) != 0) 
					{
					 	fprintf(stderr, "Err. pthread_create() %s\n", strerror(error));
						exit(EXIT_FAILURE);
					}
					// printf("request attended\n");


				}

			}
			else if(strncmp(cmd,"/er",3) == 0)
			{
				printf("There was an error, commands not executed!\n");
			}

			else if(strncmp(cmd,"/so",3) == 0)
			{

				int port;
				char dTemp[300], nTemp[300];

				// extract information from msg
				deserializer2(msg,source,msg);
				index = findNiceSpot(users,MAX_USERS);

				port = atoi(msg);
				

				memcpy(users[index].name, source, strlen(source)*sizeof(char));

				

				setPort(users,index,port);
				setRole(users,index,CLIENT);
				printf("\nRECIEVED IN /SO %s\n", msg );
				memset(users[index].d, '\0', 300*sizeof(char));
				memset(users[index].n, '\0', 300*sizeof(char));

				sscanf(msg,"%d %s %s",&port, users[index].d, users[index].n);
				setChatKeyD(users, index, users[index].d);
				setChatKeyN(users, index, users[index].n);
				// bdConvToHex(d, (users+chat)->d, BUF_SIZE);
				// bdConvToHex(n, (users+chat)->n, BUF_SIZE);

				printf("\n");
				printf("\n");

				printUserInfo(&users[index]);

				printf("\n");
				printf("\n");
				
				if ((error = pthread_create(
										&(users[index].userPThread),
										NULL,
										chat,
										(void*)&(index))) != 0) 
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

void encrypt(unsigned char *inMsg, BIGD n, BIGD e, unsigned char *outMsg)
{
	/* Create a PKCS#1 v1.5 EME message block in octet format */
		/*
		|<-----------------(klen bytes)--------------->|
		+--+--+-------+--+-----------------------------+
		|00|02|PADDING|00|      DATA TO ENCRYPT        |
		+--+--+-------+--+-----------------------------+
		The padding is made up of _at least_ eight non-zero random bytes.
		*/
	BIGD c, m;
	c = bdNew();
	m = bdNew();
	int npad, i, mlen;
	int klen = (KEYSIZE+7)/8;
	unsigned char block[(KEYSIZE+7)/8];
	unsigned char rb;
	bdPrintHex("N is=\n", n, "\n");
	bdPrintHex("E is=\n", e, "\n");
	
	/* CAUTION: make sure the block is at least klen bytes long */
	memset(block, 0, klen);
	mlen = strlen( (char*)inMsg );
	npad = klen - mlen - 3;
	if (npad < 8)	/* Note npad is a signed int, not a size_t */
	{
		printf("Message is too long\n");
		exit(1);
	}
	/* Display */
	//printf("Message='%s' ", inMsg);
	//pr_bytesmsg("0x", inMsg, strlen((char*)inMsg));

	/* Create encryption block */
	block[0] = 0x00;
	block[1] = 0x02;
	/* Generate npad non-zero padding bytes - rand() is OK */
	srand((unsigned)time(NULL));
	for (i = 0; i < npad; i++)
	{
		while ((rb = (rand() & 0xFF)) == 0)
			;/* loop until non-zero*/
		block[i+2] = rb;
	}
	block[npad+2] = 0x00;
	memcpy(&block[npad+3], inMsg, mlen);
	printf("BLOCK: %s\n", block);

	/* Convert to BIGD format */
	bdConvFromOctets(m, block, klen);

	bdPrintHex("m=\n", m, "\n");

	/* Encrypt c = m^e mod n */
	bdModExp(c, m, e, n);
	bdPrintHex("c=\n", c, "\n");
	bdConvToHex(c, outMsg, BUF_SIZE);
	printf("END of encryption %s\n", outMsg);
	//bdConvToDecimal(c, outMsg,1000*sizeof(int)/*buffer size*/);
	//return outMsg - our message
	bdFree(&c);
	bdFree(&m);
}

void Decryptor(unsigned char *inMsg, unsigned char *outMsg, BIGD d, BIGD n)
{
	unsigned char block[(KEYSIZE+7)/8];
	// char msgstr[sizeof(block)];
	int klen = (KEYSIZE+7)/8;
	//int res; 
	int i;
	int nchars;
	BIGD m1, c;
	m1 = bdNew();
	c = bdNew();

	bdConvFromHex(c, inMsg);


//	char msgstr[sizeof(block)];

	//we take outMsg and conv it to bd

	//bdConvFromDecimal(c, (const char*)outMsg);
	/* Check decrypt m1 = c^d mod n */
	
	bdModExp(m1, c, d, n);

	bdPrintHex("m'=\n", m1, "\n");
	//res = bdCompare(m1, m);
	//printf("Decryption %s\n", (res == 0 ? "OK" : "FAILED!"));
	//assert(res == 0);
	//printf("Decrypt by inversion took %.3f secs\n", tinv);

	/* Extract the message bytes from the decrypted block */
	memset(block, 0, klen);
	bdConvToOctets(m1, block, klen);
	assert(block[0] == 0x00);
	assert(block[1] == 0x02);
	for (i = 2; i < klen; i++)
	{	/* Look for zero separating byte */
		if (block[i] == 0x00)
			break;
	}
	if (i >= klen)
		printf("ERROR: failed to find message in decrypted block\n");
	else
	{
		nchars = klen - i - 1;
		memcpy(outMsg, &block[i+1], nchars);
		outMsg[nchars] = '\0';
		printf("Decrypted message is '%s'\n", outMsg);
	}
	bdFree(&m1);
	bdFree(&c);
}

int generateRSAKey(BIGD n, BIGD e, BIGD d)
{
	size_t nbits = KEYSIZE;	/* NB a multiple of 8 here */
	BIGD g, p1, q1, phi;
	BIGD p, q, dP, dQ, qInv;
	BD_RANDFUNC randFunc = bdRandomOctets;
	unsigned ee = 0x3;
	size_t np, nq;
	size_t seedlen = 0;
	size_t ntests = 50;
	unsigned char *myseed = NULL;
	unsigned char *seed = NULL;
//	clock_t start, finish;
//	double duration, tmake;
	int res;

	/* Initialise */
	g = bdNew();
	p1 = bdNew();
	q1 = bdNew();
	phi = bdNew();

	p = bdNew();
	q = bdNew();
	dP = bdNew();
	dQ = bdNew();
	qInv = bdNew();

	printf("Generating a %d-bit RSA key...\n", nbits);
	
	/* Set e as a BigDigit from short value ee */
	bdSetShort(e, ee);
	bdPrintHex("e=", e, "\n");

	/* We add an extra byte to the user-supplied seed */
	myseed = malloc(seedlen + 1);
	if (!myseed) return -1;
	memcpy(myseed, seed, seedlen);
	/* Make sure seeds are slightly different for p and q */
	myseed[seedlen] = 0x01;

	/* Do (p, q) in two halves, approx equal */
	nq = nbits / 2 ;
	np = nbits - nq;

	/* Compute two primes of required length with p mod e > 1 and *second* highest bit set */
	// start = clock();
	do {
		bdGeneratePrime(p, np, ntests, myseed, seedlen+1, randFunc);
		bdPrintHex("Try p=", p, "\n");
	} while ((bdShortMod(g, p, ee) == 1) || bdGetBit(p, np-2) == 0);
	// finish = clock();
	// duration = (double)(finish - start) / CLOCKS_PER_SEC;
	// tmake = duration;
	// printf("p is %d bits, bit(%d) is %d\n", bdBitLength(p), np-2, bdGetBit(p,np-2));

	myseed[seedlen] = 0xff;
	// start = clock();
	do {
		bdGeneratePrime(q, nq, ntests, myseed, seedlen+1, randFunc);
		bdPrintHex("Try q=", q, "\n");
	} while ((bdShortMod(g, q, ee) == 1) || bdGetBit(q, nq-2) == 0);

	// finish = clock();
	// duration = (double)(finish - start) / CLOCKS_PER_SEC;
	// tmake += duration;
	// printf("q is %d bits, bit(%d) is %d\n", bdBitLength(q), nq-2, bdGetBit(q,nq-2));
	// printf("Prime generation took %.3f secs\n", duration); 

	/* Compute n = pq */
	bdMultiply(n, p, q);
	bdPrintHex("n=\n", n, "\n");
	printf("n is %d bits\n", bdBitLength(n));
	assert(bdBitLength(n) == nbits);

	/* Check that p != q (if so, RNG is faulty!) */
	assert(!bdIsEqual(p, q));

	/* If q > p swap p and q so p > q */
	if (bdCompare(p, q) < 1)
	{	
		printf("Swopping p and q so p > q...\n");
		bdSetEqual(g, p);
		bdSetEqual(p, q);
		bdSetEqual(q, g);
	}
	bdPrintHex("p=", p, "\n");
	bdPrintHex("q=", q, "\n");

	/* Calc p-1 and q-1 */
	bdSetEqual(p1, p);
	bdDecrement(p1);
	bdPrintHex("p-1=\n", p1, "\n");
	bdSetEqual(q1, q);
	bdDecrement(q1);
	bdPrintHex("q-1=\n", q1, "\n");

	/* Compute phi = (p-1)(q-1) */
	bdMultiply(phi, p1, q1);
	bdPrintHex("phi=\n", phi, "\n");

	/* Check gcd(phi, e) == 1 */
	bdGcd(g, phi, e);
	bdPrintHex("gcd(phi,e)=", g, "\n");
	assert(bdShortCmp(g, 1) == 0);

	/* Compute inverse of e modulo phi: d = 1/e mod (p-1)(q-1) */
	res = bdModInv(d, e, phi);
	assert(res == 0);
	bdPrintHex("d=\n", d, "\n");

	/* Check ed = 1 mod phi */
	bdModMult(g, e, d, phi);
	bdPrintHex("ed mod phi=", g, "\n");
	assert(bdShortCmp(g, 1) == 0);

	/* Calculate CRT key values */
	printf("CRT values:\n");
	bdModInv(dP, e, p1);
	bdModInv(dQ, e, q1);
	bdModInv(qInv, q, p);
	bdPrintHex("dP=", dP, "\n");
	bdPrintHex("dQ=", dQ, "\n");
	bdPrintHex("qInv=", qInv, "\n");

	printf("n is %d bits\n", bdBitLength(n));

	/* Clean up */
	if (myseed) free(myseed);
	bdFree(&g);
	bdFree(&p1);
	bdFree(&q1);
	bdFree(&phi);

	bdFree(&p);
	bdFree(&q);
	bdFree(&dP);
	bdFree(&dQ);
	bdFree(&qInv);

	return 0;
}
void setDN(userInfo * user, unsigned char *msg, int i)
{
	memcpy(user->d,msg,(i)*sizeof(char));
	memcpy(user->n,msg+i+1,strlen(msg)*sizeof(char));
	printf("DONE SETTING KEYS\n");
	printf("D:::%s\n", user->d);
	printf("C:::%s\n", user->n);
}