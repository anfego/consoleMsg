#include <string.h>
#include <stdio.h>
#include "userInfo.h"
// typedef struct userInfo
// {
// 	char name[20];
// 	char ip[20];
// 	int port;
// 	int socketHandler;
// 	int status;

// } userInfo;

// gets the number conneted users
int getConnectedUsers(userInfo * users, int numElements)
{
	int counter =0;
	int i;
	for (i = 0; i < numElements; ++i)
	{
		if((users + i)->status == 1)
			counter++;
	}
	return counter;
}
// return index of a user by its name
int getUserByName(userInfo * users, char * userName, int numElements)
{
	int i;
	for (i = 0; i < numElements; ++i)
	{
		if(!strcmp((users+i)->name ,userName))
			return i;
	}
	
	return -1;

}
//
int isUserConnected(userInfo * users, char * userName,  int numElements)
{
	int i;
	for (i = 0; i < numElements; ++i)
	{
		if(!strcmp((users+i)->name ,userName) && (users+i)->status == 1)
			return 1;
	}
	
	return -1;
}

void zeroStatus(userInfo *  users,int numElements)
{
	int i;
	for (i = 0; i < numElements; ++i)
	{
		(users+i)->status = 0;
	}
}
//retunrs a good place to store the new user
//-1 if all is full
int findNiceSpot(userInfo * users,int numElements)
{
	int i;
	for (i = 0; i < numElements; ++i)
	{
		printf("actual status %d, I is %d\n",(users+i)->status, i );
		if((users+i)->status == 0)
			return i;
	}	
	//
	return -1;
}

void printAllListInfo(userInfo * users,int numElements)
{
	int i;
	for (i = 0; i < numElements; ++i)
	{
		printf("User: %s, socket: %d, status: %d \n",(users+i)->name, (users+i)->socketHandler, (users+i)->status );
	}	

}
