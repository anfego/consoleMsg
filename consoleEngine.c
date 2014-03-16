/*////////////////////////////////////////////////
File:
	consoleEngine.c
Autor:
	Andres F. Gomez
	Ivan Syzonenko
Date:
	01/19/2014
Description:
	**This is the consoleMsg engine, this program setup a TCP socket
	and listen for comming connections.
	**Store data from clients
	**Return info requested about clients

Arguments:
	

*/////////////////////////////////////////////////

//#define NDEBUG 

// #include "sockets_demo.h"


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>

//project's libraries


int console_Engine();

int main(int argc, char *argv[])
{
	int numproc = atoi(argv[1]);
	console_Engine(numproc);
	return 0;
}


int console_Engine(int numproc)
{

	fd_set active_fd_set, read_fd_set, write_fd_set;
	int i;
	struct sockaddr_in clientname;
	struct timeval tv;
	int retVal;
	/* Wait up to five seconds. */
	tv.tv_sec = 0;
	tv.tv_usec = 500;
	int pid;
	int msgSize[128] = {0};
	
	// gets a port number
	int portNumber = 0;
	int engineSocket = New_Socket(&portNumber);

	size_t size;
	
	int processDone = 0;
	int processConnect = 0;

	// Initialize the set of active sockets. 
	FD_ZERO (&active_fd_set);
	FD_SET (engineSocket, &active_fd_set);

	while (processDone < numProc)
	{
		// Block until input arrives on one or more active sockets. 
		read_fd_set = active_fd_set;
		retVal = select (FD_SETSIZE, &read_fd_set, NULL, NULL, &tv);

		if (retVal == -1)
		{
			perror ("select");
			exit (EXIT_FAILURE);
		}
			// Service all the sockets with input pending. 
			for (i = 0; i < FD_SETSIZE; ++i)
			{

				if (FD_ISSET (i, &read_fd_set))
				{

					if (i == engineSocket)
					{
						

						// Connection request on original socket. 
						int new;
						size = sizeof (clientname);
						new = accept (engineSocket,
										(struct sockaddr *) &clientname,
										&size);
						if (new < 0)
						{
							perror ("accept");
							exit (EXIT_FAILURE);
						}
						// fprintf (stderr,
						// 	"Server: connect from host %s, port %d.\n",
						// 	inet_ntoa (clientname.sin_addr),
						// 	ntohs (clientname.sin_port));
						
						FD_SET (new, &active_fd_set);
						// printf("original: %d\n", new);
						
						processConnect++;
						// sprintf(buffer,"MPI_exec %d",i);
						// FD_SET (new, &write_fd_set);

					}
					else
					{
						char buffer[1024] = {0};
						int bytesRead = 0;
						// Data arriving on an already-connected socket. 
						if (msgSize[i] == 0)
						{
							bytesRead = Recv_Msg(i,buffer);
							// printf("buffer mpi_Engine %s\n", buffer );
							//get envelope
							if ((strcmp(buffer,"MPI_Finalize") == 0) || bytesRead == RECV_EOF)
							{
							
								close (i);
								FD_CLR (i, &active_fd_set);
								processDone++;
								// printf("processDone = %d\n", i);
							}
							else
								sscanf(buffer,"%d",&msgSize[i]);
						}
						else if (msgSize[i] > 0)
						{
							bytesRead = Recv_Msg_Size(i,buffer,msgSize[i]);
						
							// printf("bytesRead: %d msgSize: %d\n", bytesRead,msgSize[i]);
						
							// printf("i: %d buffer: %s\n", i, buffer);
							
							msg_Decoder(i,buffer, numProc, msgSize[i]);
							msgSize[i] = 0;
						}
							// if (numProc == comm_world.size)
							// {
							// 	printf("COMM DONE\n");
							// 	// FD_SET()
							// }


							
							// FD_CLR (i, &active_fd_set);

					}
				}
			}

				// if (FD_ISSET (i, &write_fd_set))
				// {
				// 	// printf("Ready to Send: %d\n",i);
				// 	Send_Msg(i,buffer,400);
				// 	FD_CLR (i, &write_fd_set);


				// }
			
		
	}
	
	free(buffer);
	
	printf("BYE\n");
	return 0;
}