#include <string.h>
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
		if(users[i].status == 1)
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
		if(!strcmp(users[i].name ,userName))
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
		if(!strcmp(users[i].name ,userName) && users[i].status == 1)
			return 1;
	}
	
	return -1;
}