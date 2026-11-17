#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

int x = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void *worker(void *arg)
{
    pthread_mutex_lock(&mutex);

    if (x != -1) {
        printf("x = %d, bekliyorum...\n", x);
        pthread_cond_wait(&cond, &mutex);
    }

    printf("x = -1, uyandim!\n");

    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main()
{
    pthread_t thread;

    pthread_create(&thread, NULL, worker, NULL);

    sleep(3);

    pthread_mutex_lock(&mutex);

    x = -1;
    pthread_cond_signal(&cond);

    pthread_mutex_unlock(&mutex);

    pthread_join(thread, NULL);

    return 0;
}
