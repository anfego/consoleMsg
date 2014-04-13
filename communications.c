#include <errno.h>
#include <stdio.h>

#include "communications.h"

int New_Socket(int *port)
{
	int rc, newSocket;
	struct sockaddr_in sin;
	socklen_t newSocketL = sizeof(sin);

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(*port);

	newSocket = socket(AF_INET, SOCK_STREAM, 0);
	Error_Check(newSocket,"newSocket socket");

	rc = bind(newSocket, (struct sockaddr *)&sin ,sizeof(sin));
	Error_Check(rc,"newSocket bind");

	rc = listen(newSocket, DEFAULT_BACKLOG);
	Error_Check(rc,"newSocket listen");
	
	//get the port number on the socketDesc
	if (getsockname(newSocket, (struct sockaddr *)&sin, &newSocketL) == -1)
		*port = -1;
	else
		*port = ntohs(sin.sin_port);

	return newSocket;
}

int Connect_To(char *hostname, int port, int *fromPort)	
{
	int rc, client_socket;
	int optval = 1;
	
	struct sockaddr_in listener;
	struct hostent *hp;

	socklen_t client_socketL = sizeof(listener);


	hp = gethostbyname(hostname);
	if (hp == NULL)
		Error_Check(-1,"Connect_To");
	

	// bzero((void *)&listener, client_socketL);
	memset((void*)&listener,0,client_socketL);
	memcpy((void *)hp->h_addr, (void *)&listener.sin_addr, hp->h_length);

	
	listener.sin_family = hp->h_addrtype;
	listener.sin_port = htons(port);

	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	Error_Check(client_socket, "net_connect_to_server socket");

	rc = connect(client_socket,(struct sockaddr *) &listener, client_socketL);
	Error_Check(rc, "net_connect_to_server connect");
	setsockopt(client_socket,IPPROTO_TCP,TCP_NODELAY,(char *)&optval,sizeof(optval));

	//get the port number on the socketDesc
	if (getsockname(client_socket, (struct sockaddr *)&listener, &client_socketL) == -1)
		*fromPort = -1;
	else
		*fromPort = ntohs(listener.sin_port);

	return client_socket;
}

int Error_Check(int val, char *str)	
{
	if (val < 0)
	{
		printf("%s :%d: %s\n", str, val, strerror(errno));
		// printf("%s :%d\n", str, val);
		return -1;
	}
}

int Accept_Connection(int socket)
{
	int fromlen, new_socket, gotit;
	int optval = 1,optlen;
	struct sockaddr_in from;

	fromlen = sizeof(from);
	gotit = 0;
	while (!gotit)
	{
		new_socket = accept(socket, (struct sockaddr *)&from, &fromlen);
		if (new_socket == -1)
		{
    			// Did we get interrupted? If so, try again 
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			else
				Error_Check(new_socket, "accept_connection accept");
		}
		else
			gotit = 1;
	}
	// setsockopt(new_socket,IPPROTO_TCP,TCP_NODELAY,(char *)&optval,sizeof(optval));
	return new_socket;

}

void Send_Msg(int fd,char* buf, int size)
{
	int n;
	n = write(fd, buf, size);
	Error_Check(n, "Send_Msg write");

}

int Recv_Msg(int fd, char * buf )
{
	int bytes_read;

	bytes_read = read(fd, buf, BUFFER_LEN);
	// printf("bytes_read %d\n", bytes_read);
	Error_Check( bytes_read, "Recv_Msg read");
	
	if (bytes_read == 0)
		return RECV_EOF;

	return bytes_read;
}
int Recv_Msg_Size(int fd, char * buf, int size)
{
	int bytes_read;

	bytes_read = read(fd, buf, size);
	// printf("bytes_read %d\n", bytes_read);
	Error_Check( bytes_read, "Recv_Msg_Size read");
	
	if (bytes_read == 0)
		return RECV_EOF;

	return bytes_read;
		return bytes_read;


}
