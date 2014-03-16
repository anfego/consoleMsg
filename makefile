# I am a comment, and I want to say that the variable CC will be
# the compiler to use.
CC=gcc
# Hey!, I am comment number 2. I want to say that CFLAGS will be the
# options I'll pass to the compiler.
#CFLAGS=-c -Wall
CFLAGS = -c
AFLAGS = rf


all: server.c client.c
	$(CC) userInfo.c -c 
	$(CC) server.c  userInfo.o -o server.e  -lpthread
	$(CC) clientPThread.c userInfo.o -o client.e  -lpthread
	# gcc client.c -o client
clean:
	rm -f *.e *.o 
