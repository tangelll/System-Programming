#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    srand(time(NULL));
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char message[100];
    int p = rand()%100;
    int q =  rand()%100;

    int i = rand()%p;
    int j = rand()%q;

    // 1. Create client socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 2. Convert IPv4 address from text to binary format
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // 3. Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    sprintf(message,"Hello the area of city is at location %d , %d and my adress location is %d %d",p,q,i,j); 
    // 4. Send message to server
    send(sock, message, strlen(message), 0);
    printf("Hello message sent to server\n");

    // 5. Read response from server
    ssize_t valread = read(sock, buffer, BUFFER_SIZE - 1);
    if (valread > 0) {
        buffer[valread] = '\0';
        printf("Server says: %s\n", buffer);
    }

    // 6. Close the socket
    close(sock);

    return 0;
}

