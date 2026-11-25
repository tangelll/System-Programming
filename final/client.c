
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#define BUFFER_SIZE 1024

typedef struct {
    int id;
    int sock;
    int x;
    int y;
    int active;
    int order_sent;
    char buffer[BUFFER_SIZE];
} Client;

volatile sig_atomic_t cancel_requested = 0;

void signal_handler(int sig)
{
    cancel_requested = 1;
}

/* Mesajin tamamini gondermeye calisir */
int send_all(int sock, const char *message)
{
    size_t total = 0;
    size_t length = strlen(message);

    while (total < length) {
        ssize_t sent = send(sock, message + total,
                            length - total, MSG_NOSIGNAL);

        if (sent <= 0)
            return -1;

        total += sent;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 6) {
        printf("Usage: %s [IP] [port] [numberOfClients] [p] [q]\n",
               argv[0]);
        return EXIT_FAILURE;
    }

    /* Argumanlar */
    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    int numberOfClients = atoi(argv[3]);
    int p = atoi(argv[4]);
    int q = atoi(argv[5]);

    if (port <= 0 || numberOfClients <= 0 || p <= 0 || q <= 0) {
        printf("Invalid arguments\n");
        return EXIT_FAILURE;
    }

    srand(time(NULL));

    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    /* Client dizisi */
    Client *clients = calloc(numberOfClients, sizeof(Client));

    /* Poll dizisi */
    struct pollfd *fds = calloc(numberOfClients + 1,
                                sizeof(struct pollfd));

    if (clients == NULL || fds == NULL) {
        perror("calloc");
        free(clients);
        free(fds);
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip,
                  &server_addr.sin_addr) <= 0) {
        printf("Invalid server address\n");
        free(clients);
        free(fds);
        return EXIT_FAILURE;
    }

    /* Her musteri icin ayri socket ve baglanti */
    for (int i = 0; i < numberOfClients; i++) {

        clients[i].id = i + 1;
        clients[i].x = rand() % p;
        clients[i].y = rand() % q;

        clients[i].sock = socket(AF_INET, SOCK_STREAM, 0);

        if (clients[i].sock < 0) {
            perror("socket");
            goto cleanup;
        }

        if (connect(clients[i].sock,
                    (struct sockaddr *)&server_addr,
                    sizeof(server_addr)) < 0) {
            perror("connect");
            goto cleanup;
        }

        clients[i].active = 1;

        printf("Client %d connected. PID: %d\n",
               clients[i].id, getpid());

        /*
         * Her musterinin siparis mesaji.
         * Mesaj protokolunu server tarafiyla
         * ayni hale getirmeliyiz.
         */
        char message[BUFFER_SIZE];

        snprintf(message, sizeof(message),
                 "ORDER %d %d %d\n",
                 clients[i].id,
                 clients[i].x,
                 clients[i].y);

        if (send_all(clients[i].sock, message) < 0) {
            perror("send");
            close(clients[i].sock);
            clients[i].active = 0;
            continue;
        }

        clients[i].order_sent = 1;

        printf("Client %d sent order: (%d   %d )\n",
               clients[i].id,
               clients[i].x,
               clients[i].y);
    }

    /*
     * Poll dizisi:
     * fds[0] = Terminal input
     * fds[1..N] = Client socketleri
     */

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    for (int i = 0; i < numberOfClients; i++) {
        fds[i + 1].fd = clients[i].active  ? clients[i].sock : -1;

        fds[i + 1].events = POLLIN;
    }

    printf("\nWaiting for server responses...\n");

    int active_count = 0;

    for (int i = 0; i < numberOfClients; i++) {
        if (clients[i].active)
            active_count++;
    }

    while (active_count > 0) {

        if (cancel_requested) {
            printf("\nSIGINT received. Cancelling orders...\n");

            for (int i = 0; i < numberOfClients; i++) {
                if (clients[i].active) {
                    char cancel_msg[100];

                    snprintf(cancel_msg, sizeof(cancel_msg),
                             "CANCEL %d\n", clients[i].id);

                    send_all(clients[i].sock, cancel_msg);
                }
            }

            break;
        }

        int result = poll(fds, numberOfClients + 1, -1);

        if (result < 0) {
            if (errno == EINTR)
                continue;

            perror("poll");
            break;
        }

        /* Ctrl+D kontrolu */
        if (fds[0].revents & POLLIN) {
            char input;
            ssize_t n = read(STDIN_FILENO, &input, 1);

            if (n == 0) {
                printf("\nEOF received. Cancelling orders...\n");

                for (int i = 0; i < numberOfClients; i++) {
                    if (clients[i].active) {
                        char cancel_msg[100];

                        snprintf(cancel_msg, sizeof(cancel_msg),
                                 "CANCEL %d\n", clients[i].id);

                        send_all(clients[i].sock, cancel_msg);
                    }
                }

                break;
            }
        }

        /* Client socketlerini kontrol et */
        for (int i = 0; i < numberOfClients; i++) {

            if (!clients[i].active)
                continue;

            if (fds[i + 1].revents & POLLIN) {

                char buffer[BUFFER_SIZE];

                ssize_t received = recv(
                    clients[i].sock,
                    buffer,
                    sizeof(buffer) - 1,
                    0
                );

                if (received > 0) {
                    buffer[received] = '\0';

                    printf("Client %d received: %s", clients[i].id, buffer);
                    
                    int num;
                    sscanf(buffer, "ORDER_RECEIVED %d", &num);
                    if (num == numberOfClients ) {
                        goto cleanup;
                    }
                }
                else if (received == 0) {
                    printf("Client %d disconnected from server\n",
                           clients[i].id);

                    close(clients[i].sock);
                    clients[i].active = 0;
                    fds[i + 1].fd = -1;
                    active_count--;
                }
                else if (errno != EINTR) {
                    perror("recv");

                    close(clients[i].sock);
                    clients[i].active = 0;
                    fds[i + 1].fd = -1;
                    active_count--;
                }
            }

            if (clients[i].active &&
                (fds[i + 1].revents & (POLLERR | POLLHUP | POLLNVAL))) {

                printf("Client %d connection closed\n",
                       clients[i].id);

                close(clients[i].sock);
                clients[i].active = 0;
                fds[i + 1].fd = -1;
                active_count--;
            }
        }
    }

cleanup:

    for (int i = 0; i < numberOfClients; i++) {
        if (clients[i].active)
            close(clients[i].sock);
    }

    free(clients);
    free(fds);

    printf("HungryVeryMuch terminated.\n");

    return EXIT_SUCCESS;
}

