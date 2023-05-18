all:
	gcc -Wall -c common.c
	gcc -Wall client.c common.o -o client
	gcc -Wall clientCopy.c common.o -o clientCopy
	gcc -Wall server.c common.o -o server
	gcc -Wall serverCopy.c common.o -o serverCopy