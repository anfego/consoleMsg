#ifndef USER_H
#define USER_H

#include <string.h>


typedef struct userInfo
{
	char name[20];
	char ip[20];
	int port;
	int socketHandler;
	int status;

} userInfo;

// gets the number conneted users
int getConnectedUsers(userInfo * users);
// return index of a user by its name
int getUserByName(userInfo * users, char * userName);
//
int isUserConnected(userInfo * users, char * userName);

#endif