#clang program.c -o program
#clang program.c -o program -pthread
all:
	clang server.c -o server
	clang client.c -o client

