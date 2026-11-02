#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define THREAD_COUNT 5
#define JOB_COUNT 5
#define PORT 8080
#define BUFFER_SIZE 1024

int jobs[JOB_COUNT] = {1,2,3,4,5};
int next_job = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg)
{
    int thread_id = *(int *) arg;
    while(1)
    {
        pthread_mutex_lock(&mutex);
        
        if(next_job >= JOB_COUNT)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }
        int job = jobs[next_job];
        next_job++;
        
        pthread_mutex_unlock(&mutex);
        printf("Thread %d, is %d isi isliyor\n",thread_id,job);
        
    }
    return NULL;
}

int main()
{   int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    char *hello = "Hello from server";
    
    if((server_fd = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt =1;
    if (setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR, &opt,sizeof(opt)) < 0 )
    {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if(bind(server_fd,(struct sockaddr *)&address,sizeof(address))<0)
    {   
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd,SOMAXCONN) < 0)
    {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listanning on port %d... \n",PORT);

    if((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t*)&addrlen)) < 0)
    {
        perror("Accept failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    ssize_t valread = read(new_socket,buffer,BUFFER_SIZE-1);
    if(valread >0)
    {
        int sehir_i;
        int sehir_j;

        int must_i;
        int must_j;
        buffer[valread] = '\0';
        sscanf(buffer,"Hello the area of city is at location %d , %d and my adress location is %d %d",&sehir_i,&sehir_j,&must_i,&must_j);
        printf("sehir kord. %d %d , musteri kord %d %d\n",sehir_i,sehir_j,must_i,must_j);
    }

    send(new_socket,hello,strlen(hello),0);

    close(new_socket);
    close(server_fd);



    pthread_t threads[THREAD_COUNT];
    int ids[THREAD_COUNT];

    for (int i = 0; i<THREAD_COUNT;i++)
    {
        ids[i] = i+1;
        if(pthread_create(&threads[i],NULL,worker,&ids[i]) != 0)
        {
            perror("error in create thread");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < THREAD_COUNT;i++)
    {
        pthread_join(threads[i],NULL);
    }

    pthread_mutex_destroy(&mutex);
    return 0;
}
