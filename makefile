all: server.c client.c
	gcc server.c -o server  -lpthread
	gcc clientPThread.c -o clientPT  -lpthread
	gcc client.c -o client
clean:
	rm -f client server serverPT
