#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 200
#define BUFFER_SIZE 1024
#define COOKER_COUNT 4
#define DELIVERY_COUNT 6




volatile sig_atomic_t keep_running = 1;
char siparis_listesi [200][100] = {0};
int front = 0;
int rear = 0;
int count = 0;

pthread_mutex_t siparis_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t siparis_uyan = PTHREAD_COND_INITIALIZER;
pthread_mutex_t kurye_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t kurye_uyan = PTHREAD_COND_INITIALIZER;
int siparis_var = 0;
int teslimat_var =0;

typedef struct {
    int fd;
    char buffer[BUFFER_SIZE];
    int used;
} Client;

typedef struct {
    int capacity;
    int door;
    int kurek;

    pthread_mutex_t mutex_door;
    pthread_mutex_t mutex_kurek;
    pthread_mutex_t mutex_capacity;
    pthread_cond_t cond_door;
    pthread_cond_t cond_kurek;
    pthread_cond_t cond_capacity;

} Oven;

typedef struct {
    pthread_t *threads;
    int thread_count;
} ThreadPool;

void siparis_ekle(char *siparis);
void siparis_isle();
void *delivery_worker(void *arg);
void close_client(Client *client);
void *cooker_worker(void *arg);
void signalHandler(int sig);
int send_all(int fd, const char *message);
void process_message(int fd, char *message);
void oven_init(Oven *oven);
void thread_pool_init(ThreadPool *pool, int thread_count);
void thread_pool_destroy(ThreadPool *pool);
void print_siparis_listesi();

ThreadPool cooker_pool;
ThreadPool delivery_pool;
Oven my_oven;
Client clients[MAX_CLIENTS];
int temp_id;


void siparis_ekle(char *siparis)
{
    if (count == 200) {
        printf("Queue dolu!\n");
        return;
    }

    strcpy(siparis_listesi[rear], siparis);

    rear = (rear + 1) % 200;
    count++;
}

void print_siparis_listesi()
{
    printf("siparis listsi");
    for(int i=0; i < 200; i++)
    {
        //printf(" %d",siparis_listesi[i]);
    }
}
void siparis_isle()
{
    if (count == 0) {
        printf("Queue bos!\n");
    }


    front = (front + 1) % 200;
    count--;
    teslimat_var =1;
    pthread_cond_signal(&kurye_uyan);
}


void *delivery_worker(void *arg)
{
    while(keep_running)
    {
        pthread_mutex_lock(&kurye_mutex);

        while (teslimat_var == 0 && keep_running) {
            printf("Asci siparis bekliyor...\n");
            pthread_cond_wait(&kurye_uyan, &kurye_mutex);
        }

        printf("kurye teslimati aldi\n");
        usleep(158);
        teslimat_var =0;
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response), "ORDER_RECEIVED %d\n",temp_id);
        send_all(temp_id, response);
        printf("kurye gonderdi \n");
        pthread_mutex_unlock(&kurye_mutex);

    }
   
    return NULL;
}



void *cooker_worker(void *arg)
{
    while(keep_running)
    {
        pthread_mutex_lock(&siparis_mutex);

        while (siparis_var == 0 && keep_running) {
            printf("Asci siparis bekliyor...\n");
            pthread_cond_wait(&siparis_uyan, &siparis_mutex);
        }

        if (keep_running == 0) {
            pthread_mutex_unlock(&siparis_mutex);
            break;
        }
        if (keep_running == 0) {
            pthread_mutex_unlock(&my_oven.mutex_kurek);
            break;
        }
        if (keep_running == 0) {
            pthread_mutex_unlock(&my_oven.mutex_door);
            break;
        }
        if (keep_running == 0) {
            pthread_mutex_unlock(&my_oven.mutex_capacity);
            break;
        }

        pthread_t tid = pthread_self();
        printf("Asci %lu  siparisi aldi!\n",(unsigned long)tid);

        siparis_var = 0 ;

        pthread_mutex_unlock(&siparis_mutex);
        
        //kurek alisverisi
        pthread_mutex_lock(&my_oven.mutex_kurek);
        
        while (my_oven.kurek <= 0 && keep_running) {
            printf("Asci kurek bekliyor...\n");
            pthread_cond_wait(&my_oven.cond_kurek, &my_oven.mutex_kurek);
        }
        
        my_oven.kurek--;
        printf("Asci kuregi aldi!\n");
        usleep(58);





        //kapi alisverisi
        pthread_mutex_lock(&my_oven.mutex_door);
        while (my_oven.door <= 0 && keep_running) {
            printf("Asci firin kapisi bekliyor...\n");
            pthread_cond_wait(&my_oven.cond_door, &my_oven.mutex_door);
        }
        my_oven.door--;
        printf("Asci firin kapisini acti!\n");
        usleep(58);


        pthread_mutex_lock(&my_oven.mutex_capacity);
        while (my_oven.capacity <= 0 && keep_running) {
            printf("Asci firin capasitesi azalmasi bekliyor...\n");
            pthread_cond_wait(&my_oven.cond_capacity, &my_oven.mutex_capacity);
        }
        my_oven.capacity--;
        printf("Asci firini kullaniyor!\n");

        my_oven.door++;
        my_oven.kurek++;
        pthread_mutex_unlock(&my_oven.mutex_door);
        pthread_mutex_unlock(&my_oven.mutex_kurek);


        siparis_isle();
        usleep(158);
        my_oven.capacity++;
        pthread_mutex_unlock(&my_oven.mutex_capacity);

                
    }
    return NULL;
}

void close_client(Client *client)
{
    if (client->fd >= 0) {
        close(client->fd);
        client->fd = -1;
        client->used = 0;
    }
}

void signalHandler(int sig)
{
    keep_running = 0;
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

    if (sscanf(message, "ORDER %d %d %d  ", &id, &x, &y  ) == 3) {

        temp_id = id;
        siparis_ekle(message);
        siparis_var = 1;
        pthread_cond_signal(&siparis_uyan);

    }
    else {
        printf("Gecersiz mesaj: %s\n", message);
    }
}


void thread_pool_init(ThreadPool *pool, int thread_count)
{
    pool->thread_count = thread_count;

    pool->threads = malloc(thread_count * sizeof(pthread_t));

}

void thread_pool_destroy(ThreadPool *pool)
{
    for (int i = 0; i < pool->thread_count; i++)
    {
        pthread_join(pool->threads[i], NULL);
    }

    free(pool->threads);
}

int main(void)
{
    signal(SIGINT, signalHandler);
    signal(SIGPIPE, SIG_IGN);

    
    // Fırını başlat
    my_oven.door =2;
    my_oven.kurek =3;
    my_oven.capacity = 6;

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

    if(keep_running == 0)
    {
       pthread_mutex_lock(&siparis_mutex);
        pthread_mutex_lock(&kurye_mutex);


        pthread_cond_broadcast(&siparis_uyan);
        pthread_cond_broadcast(&kurye_uyan);

        pthread_mutex_unlock(&siparis_mutex);
        pthread_mutex_unlock(&kurye_mutex);

    }
    printf("Server shutting down...\n");

    for (int i = 0; i < MAX_CLIENTS; i++)
        close_client(&clients[i]);

    thread_pool_destroy(&cooker_pool);
    thread_pool_destroy(&delivery_pool);

    close(server_fd);

    return 0;
}
