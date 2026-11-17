#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>

#define PORT 8080
#define MAX_CLIENTS 200
#define BUFFER_SIZE 1024
#define COOKER_COUNT 4
#define DELIVERY_COUNT 6
#define KUREK_NUM 3
#define OVEN_CAPACITY 6
#define OVEN_OPENNINGS 2

volatile sig_atomic_t keep_running = 1;

typedef struct {
    int fd;
    char buffer[BUFFER_SIZE];
    int used;
} Client;

typedef struct {
    int capacity[OVEN_CAPACITY];
    int openings[OVEN_OPENNINGS];
    int kurek[KUREK_NUM];

    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Oven;

typedef struct QueueNode {
    Order order;
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *front;
    QueueNode *rear;
    int count;
    int shutdown;

    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Queue;

typedef struct {
    pthread_t *threads;
    int thread_count;
    Queue queue;
} ThreadPool;

ThreadPool cooker_pool;
ThreadPool delivery_pool;
Oven my_oven;


int queue_pop(Queue *queue, Order *order)
{
    pthread_mutex_lock(&queue->mutex);

    while (queue->count == 0 && !queue->shutdown)
    {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }

    if (queue->count == 0 && queue->shutdown)
    {
        pthread_mutex_unlock(&queue->mutex);
        return 0;
    }

    // Kuyruğun başındaki siparişi al
    // front ve count değerlerini güncelle

    pthread_mutex_unlock(&queue->mutex);
    return 1;
}

void queue_push(Queue *queue, Order order)
{
    QueueNode *new_node = malloc(sizeof(QueueNode));

    if (new_node == NULL)
        return;

    new_node->order = order;
    new_node->next = NULL;

    pthread_mutex_lock(&queue->mutex);

    if (queue->rear == NULL)
    {
        queue->front = new_node;
        queue->rear = new_node;
    }
    else
    {
        queue->rear->next = new_node;
        queue->rear = new_node;
    }

    queue->count++;

    pthread_cond_signal(&queue->cond);

    pthread_mutex_unlock(&queue->mutex);
}

void *cooker_worker(void *arg)
{
    ThreadPool *pool = arg;
    Order order;

    while (queue_pop(&pool->queue, &order))
    {
        // Siparişi hazırla
    }

    return NULL;
}

void signalHandler(int sig)
{
    keep_running = 0;
}

void close_client(Client *client)
{
    if (client->fd >= 0) {
        close(client->fd);
        client->fd = -1;
        client->used = 0;
    }
}

/* Tam mesaj gonder */
int send_all(int fd, const char *message)
{
    size_t total = 0;
    size_t length = strlen(message);

    while (total < length) {
        ssize_t sent = send(fd, message + total,
                            length - total, 0);

        if (sent <= 0)
            return -1;

        total += sent;
    }

    return 0;
}

/* Tamamlanmis bir siparis mesajini isle */
void process_message(int fd, char *message)
{
    int id, x, y;

    if (sscanf(message, "ORDER %d %d %d", &id, &x, &y) == 3) {

        printf("Client %d, konum: %d %d\n", id, x, y);

        char response[BUFFER_SIZE];

        snprintf(response, sizeof(response),
                 "ORDER_RECEIVED %d\n", id);

        send_all(fd, response);
    }
    else {
        printf("Gecersiz mesaj: %s\n", message);
    }
}

void queue_init(Queue *queue)
{
    queue->front = NULL;
    queue->rear = NULL;
    queue->count = 0;

    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
}
void oven_init(Oven *oven)
{
    memset(oven->capacity, 0, sizeof(oven->capacity));
    memset(oven->openings, 0, sizeof(oven->openings));
    memset(oven->kurek, 0, sizeof(oven->kurek));

    pthread_mutex_init(&oven->mutex, NULL);
    pthread_cond_init(&oven->cond, NULL);
}
void thread_pool_init(ThreadPool *pool, int thread_count)
{
    pool->thread_count = thread_count;

    pool->threads = malloc(thread_count * sizeof(pthread_t));

    queue_init(&pool->queue);
}

int main(void)
{
    signal(SIGINT, signalHandler);
    signal(SIGPIPE, SIG_IGN);

    
    // Fırını başlat
    oven_init(&my_oven);

    // Thread pool'ları initialize et
    thread_pool_init(&cooker_pool, COOKER_COUNT);
    thread_pool_init(&delivery_pool, DELIVERY_COUNT);

    // Cooker thread'lerini oluştur
    for (int i = 0; i < cooker_pool.thread_count; i++) {
        pthread_create(
            &cooker_pool.threads[i],
            NULL,
            cooker_worker,
            &cooker_pool
        );
    }

    // Delivery thread'lerini oluştur
    for (int i = 0; i < delivery_pool.thread_count; i++) {
        pthread_create(
            &delivery_pool.threads[i],
            NULL,
            delivery_worker,
            &delivery_pool
        );
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt));

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    Client clients[MAX_CLIENTS];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].used = 0;
    }

    printf("Server listening on port %d...\n", PORT);



    while (keep_running) {

        struct pollfd fds[MAX_CLIENTS + 1];

        /* fds[0]: yeni baglantilar */
        fds[0].fd = server_fd;
        fds[0].events = POLLIN;
        fds[0].revents = 0;

        /* fds[1..MAX_CLIENTS]: mevcut clientlar */
        for (int i = 0; i < MAX_CLIENTS; i++) {
            fds[i + 1].fd = clients[i].fd;
            fds[i + 1].events = POLLIN;
            fds[i + 1].revents = 0;
        }

        int result = poll(fds, MAX_CLIENTS + 1, -1);

        if (result < 0) {
            if (errno == EINTR)
                continue;

            perror("poll");
            break;
        }

        /* Yeni client baglantisi geldi mi? */
        if (fds[0].revents & POLLIN) {

            struct sockaddr_in client_addr;
            socklen_t addrlen = sizeof(client_addr);

            int new_socket = accept(
                server_fd,
                (struct sockaddr *)&client_addr,
                &addrlen
            );

            if (new_socket >= 0) {

                int slot = -1;

                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].fd == -1) {
                        slot = i;
                        break;
                    }
                }

                if (slot >= 0) {
                    clients[slot].fd = new_socket;
                    clients[slot].used = 0;

                    printf("New client connected: %s:%d (slot %d)\n",
                           inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port),
                           slot);
                }
                else {
                    printf("Maximum client capacity reached.\n");
                    close(new_socket);
                }
            }
        }

        /* Mevcut clientlardan gelen verileri kontrol et */
        for (int i = 0; i < MAX_CLIENTS; i++) {

            if (clients[i].fd == -1)
                continue;

            if (fds[i + 1].revents & POLLIN) {

                char temp[BUFFER_SIZE];

                ssize_t received = recv(
                    clients[i].fd,
                    temp,
                    sizeof(temp),
                    0
                );

                if (received <= 0) {
                    printf("Client disconnected: slot %d\n", i);
                    close_client(&clients[i]);
                    continue;
                }

                /* TCP verisini satir satir isle */
                for (ssize_t j = 0; j < received; j++) {

                    if (temp[j] == '\n') {

                        clients[i].buffer[clients[i].used] = '\0';

                        process_message(
                            clients[i].fd,
                            clients[i].buffer
                        );

                        clients[i].used = 0;
                    }
                    else if (clients[i].used < BUFFER_SIZE - 1) {

                        clients[i].buffer[clients[i].used++] = temp[j];
                    }
                    else {
                        printf("Client message too long. Closing.\n");
                        close_client(&clients[i]);
                        break;
                    }
                }
            }

            if (clients[i].fd != -1 &&
                (fds[i + 1].revents & (POLLERR | POLLHUP | POLLNVAL))) {

                printf("Client connection closed: slot %d\n", i);
                close_client(&clients[i]);
            }
        }
    }

    printf("Server shutting down...\n");

    for (int i = 0; i < MAX_CLIENTS; i++)
        close_client(&clients[i]);

    void thread_pool_destroy(ThreadPool *pool)
    {
        pthread_mutex_lock(&pool->queue.mutex);

        pool->queue.shutdown = 1;

        pthread_cond_broadcast(&pool->queue.cond);

        pthread_mutex_unlock(&pool->queue.mutex);

        for (int i = 0; i < pool->thread_count; i++)
        {
            pthread_join(pool->threads[i], NULL);
        }

        free(pool->threads);

        pthread_cond_destroy(&pool->queue.cond);
        pthread_mutex_destroy(&pool->queue.mutex);
    }

    thread_pool_destroy(&cooker_pool);
    thread_pool_destroy(&delivery_pool);

    close(server_fd);

    return 0;
}
