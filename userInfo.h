#ifndef USER_H
#define USER_H

#include <string.h>

#define MAX_USERS 40

typedef struct userInfo
{
	char name[20];
	//char ip[20];
	//int port;
	int socketHandler;
	int status;	//0 is not conected 1: is connected


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
#endif