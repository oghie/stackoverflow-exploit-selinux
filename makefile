all:
	gcc -g -mpreferred-stack-boundary=2 -o server server.c -lpthread
	gcc -g -mpreferred-stack-boundary=2 -o exploit exploit.c
	gcc -S -mpreferred-stack-boundary=2 -o exploit.s exploit.c
	gcc -g -mpreferred-stack-boundary=2 -o xstack xstack.c
	./xstack server

