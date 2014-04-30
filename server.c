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
#include <assert.h>
#include <ifaddrs.h>	
				

#include <arpa/inet.h>
#include "communications.h"
#include "bigd.h"
#include "bigdRand.h"




//
#include "userInfo.h"

#define PROTOPORT 9047 /* default protocol port number */
#define QLEN 6 /* size of request queue */
#define MAX_THREAD 40
#define KEYSIZE 1024	
#define BUF_SIZE 300	

int visits = 0; /* counts client connections */

void * socRead (void * args);
void * SendAll(void * args);
/*******************************************************/
// Encryption vars
BIGD n, e, d;
/*******************************************************/
void getMyIp(char * ip, int socket);
int sendMsg(char * msg, int socket, char * cmd);


void closeAll();
void rqNewConnection(int socket, char *source, int nameLen);
void deserializer(const char * buf,char * source,char * cmd,char * msg);
void deserializer2(const char * buf,char * source,char * msg);
void setDN(userInfo * user, unsigned char *msg, int i);
int generateRSAKey(BIGD n, BIGD e, BIGD d);
void Decryptor(unsigned char *inMsg, unsigned char *outMsg, BIGD d, BIGD n);
void encrypt(unsigned char *inMsg, BIGD n, BIGD e, unsigned char *outMsg);

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
	// int n = 0;
	int i=0;
	int pid;
	int thr_err;
	zeroStatus(users,MAX_USERS);
	n = bdNew();
	e = bdNew();
	d = bdNew();
	// generateRSAKey(n, e, d);
	
	
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
	bdFree(&n);
	bdFree(&e);
	bdFree(&d);
	return 0;
}

void * socRead(void * args)
{
	char buf[1500]; /* buffer for string the server sends */
	char buf2[1000]; /* buffer for string the server sends */
	char cmd[4];
	char source[20];
	char msg[1400];
	char msg2[1000];
	struct sbuf_t targ = *(struct sbuf_t *) args;
	int sd2 = targ.sda;
	int myId = targ.numCon;
	int i;

	int n = 0;
	struct sockaddr_storage sender;
	socklen_t sendsize = sizeof(sender);
	
	while (1) 
	{
		memset(buf,'\0',1500*sizeof(char));
		memset(buf2,'\0',1000*sizeof(char));

		// bzero(&sender, sizeof(sender));
		//extracts the command from the IN buffer
		n = recv(sd2, buf, sizeof(buf), 0);
		if (n > 0)
		{
			printf("\nEndOfOneWhileCycle%s\n",buf);
			memset(cmd,'\0',4*sizeof(char));
			memset(source,'\0',20*sizeof(char));
			memset(msg,'\0',1400*sizeof(char));
			
			deserializer(buf,source,cmd,msg);
			

			printf("CMD: %s\n", cmd );
			printf("Source: %s\n", source );
			printf("MSG: %s\n", msg );
			
			int index = -1;
			int iSource = -1;
			char name[20];
			int nameLen = 0;
			//this is a login
			if(strncmp(cmd,"/lg",3) == 0)
			{
				nameLen = strlen(source)+1;
				memcpy(name,source,nameLen);
				
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
						memcpy(users[index].name,source,nameLen*sizeof(char));
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
				for(i=0;i<strlen(msg);++i)
				{
					// printf("%d %s\n", i,localBuf+i);	
					if(strncmp(msg+i,"@",1) == 0)
					{
						break;
					}

				}
				if(i == strlen(msg))
				{
					printf("ERROR!\n");
					
				}
				else
				{
					setDN(&users[index], msg, i);
					
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
				// printAllListInfo(users,MAX_USERS);
				nameLen = strlen(source)+1;
				index = isUserConnected(users,msg,MAX_USERS);
				if(index >= 0)
				{
					//accepted
					printf("Request connection to %s\n",users[index].name );
					printf("Request connection from %s\n",source);
					memset(buf,'\0',1500*sizeof(char));
					iSource = getUserByName(users,source,MAX_USERS);
					sprintf(buf,"%s %s %s",source, users[iSource].d,users[iSource].n  );
					sendMsg(buf, users[index].socketHandler,"/rq");
					// rqNewConnection(users[index].socketHandler, source, nameLen);
				}
				else
				{

					printf("The User is not Connected\n");
					iSource = getUserByName(users,source,MAX_USERS);
					sendError(users[iSource].socketHandler);
				}
			}
			else if(strncmp(cmd,"/so",3) == 0)		// socket ok, created
			{
				//this a client requesting new chat

				memcpy(buf2,source,strlen(source)*sizeof(char));
				memcpy(buf2+strlen(buf2),"#",sizeof(char));

				memset(msg2,'\0',1000*sizeof(char));

				index = getUserByName(users,source,MAX_USERS);

				deserializer2(msg,source,msg2);

				printf("FROM: %s\n", source );
				
				printf("MSG 2: %s\n", msg2 );
				
				memcpy(buf2+strlen(buf2),msg2,strlen(msg2)*sizeof(char));

				iSource = getUserByName(users,source,MAX_USERS);
				sprintf(buf2 + strlen(buf2),"#%s %s",users[index].d,users[index].n  );
				printf("buf2: %s\n", buf2 );
				sendMsg(buf2, users[iSource].socketHandler,"/so");
				
			}


			
		// #endregion
			
			// printf("\033[22;31mclient: \033[22;37m");


			fflush(stdout);
			n=0;
		}
	}	
}
void setDN(userInfo * user, unsigned char *msg, int i)
{
	memcpy(user->d,msg,(i)*sizeof(char));
	memcpy(user->n,msg+i+1,strlen(msg)*sizeof(char));
	printf("DONE SETTING KEYS\n");
	printf("D:::%s\n", user->d);
	printf("C:::%s\n", user->n);
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
void rqNewConnection(int socket, char * source, int nameLen)
{
	char buf2[140] = "/rq";
	memcpy(buf2+3, source, nameLen*sizeof(char));
	printf("I am going to send : %s\n", buf2);

	send(socket,buf2,strlen(buf2),0);
	

}
void sendNewSocket(int socket, int newSocket)
{
	char buf2[140];
	char buf3[10];
	
	memset(buf2,'\0',140*sizeof(char));
	memset(buf3,'\0',10*sizeof(char));
	
	// itoa(newSocket,buf3,10);
	snprintf(buf3, 10,"%d",newSocket);
	memcpy(buf2,"/so",3*sizeof(char));
	memcpy(buf2+3,buf3,strlen(buf3)*sizeof(char));
	printf("sendNewSocket: %s\n", buf2 );
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
	char localBuf[1500];
	memset(localBuf, '\0', 1500*sizeof(char));
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
		memcpy(source,localBuf,i*sizeof(char));

	}


}

int sendMsg(char * msg, int socket,char * cmd)
{
	char myIp[140];
	char bufOut[2000];
	
	memset(bufOut,'\0',2000*sizeof(char));
	
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
	
	printf("\tbufOut Total %s\n", bufOut );
	return 0;
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