# I am a comment, and I want to say that the variable CC will be
# the compiler to use.
CC=gcc
# Hey!, I am comment number 2. I want to say that CFLAGS will be the
# options I'll pass to the compiler.
#CFLAGS=-c -Wall
CFLAGS = -c -wall
AFLAGS = rf

all: server client
 
server: server.c communications.o userInfo.o
	$(CC) server.c userInfo.o -o server -lpthread
	

client: clientPThread.o communications.o userInfo.o
	$(CC) clientPThread.c  userInfo.o communications.o -o client  -lrt  -lpthread 


%.o: %.c
	$(CC) -c $<
	# gcc client.c -o client
clean:
	rm -f *.e *.o 
