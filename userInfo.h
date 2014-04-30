#ifndef USER_H
#define USER_H

#include <string.h>
#include <pthread.h>	// use thread functions

#define MAX_USERS 	40
#define SERVER 		0
#define CLIENT 		1

typedef struct userInfo
{
	char name[20];
	//char ip[20];
	int port;
	int socketHandler;
	int status;	//0 is not conected 1: is connected
	int myRole; //0 if my role is server, 1 if my role is Client
	pthread_t userPThread;
	unsigned char d[300];
	unsigned char n[300];


} userInfo;

// gets the number conneted users
int getConnectedUsers(userInfo * users, int lenUsers);
// return index of a user by its name
int getUserByName(userInfo * users, char * userName,int lenUsers);
//returns the status of a user
int isUserConnected(userInfo * users, char * userName, int lenUsers);

void zeroStatus(userInfo *  users,int numElements);
//retunrs a good place to store the new user
//-1 if all is full
int findNiceSpot(userInfo * users,int numElements);

void printAllListInfo(userInfo * users,int numElements);
void printUserInfo(userInfo * user);
int amIClient(userInfo * chatInfo);
void setRole(userInfo * users, int chatIndex, int role);
void setPort(userInfo * users, int chatIndex, int port);
void setStatus(userInfo * users, int chatIndex);
void clearStatus(userInfo * users, int chatIndex);

#endif