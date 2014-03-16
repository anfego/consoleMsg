#include <string.h>

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
	for (int i = 0; i < numElements; ++i)
	{
		if(user[i].status == 1)
			counter++;
	}
	return counter;
}
// return index of a user by its name
int getUserByName(userInfo * users, char * userName, int numElements)
{
	for (int i = 0; i < numElements; ++i)
	{
		if(!strcmp(user[i].name ,userName))
			return i;
	}
	
	return -1;

}
//
int isUserConnected(userInfo * users, char * userName)
{
	for (int i = 0; i < numElements; ++i)
	{
		if(!strcmp(user[i].name ,userName) && user[i].status == 1)
			return 1;
	}
	
	return -1;
}