#clang program.c -o program
#clang program.c -o program -pthread
all:
	clang server.c -o server -pthread
	clang client.c -o client -pthread

#CFLAGS = -Wall -Wextra -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
CFLAGS = -Wall -Wextra -g -O1 -fsanitize=address -fno-omit-frame-pointer
clean:
	rm server
	rm client

