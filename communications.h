#ifndef COMMUNICATIONS_H
#define COMMUNICATIONS_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>


#define RECV_OK             0
#define RECV_EOF            -1
#define DEFAULT_BACKLOG     5
#define NON_RESERVED_PORT   6635
#define BUFFER_LEN          1024    
#define SERVER_ACK          "ACK_FROM_SERVER"

/* Macros to convert from integer to net byte order and vice versa */
#define i_to_n(n)  (int) htonl( (u_long) n)
#define n_to_i(n)  (int) ntohl( (u_long) n)

int New_Socket(int *port);
int Connect_To(char *hostname, int port, int *fromPort);
int Error_Check(int val, char *str);
void Send_Msg(int fd,char * buf, int size);
int Accept_Connection(int socket);
int Recv_Msg(int fd, char * buf );
int Recv_Msg_Size(int fd, char * buf,int size);
// server();
// int waitFor_MPI_Init(int default_socket, int * new_socket);
// serve_one_connection(int new_socket);
// client(char *server_host);
// setup_to_accept(int port);
// int accept_connection(int default_socket);
// connect_to_server(char *hostname, int port);
// int recv_msg(int fd, char *buf);
// send_msg(int fd, char *buf, int size);
// error_check(int val, char *str)	;

#endif
    
