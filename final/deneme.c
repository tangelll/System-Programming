#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#define THREAD_COUNT 3
#define JOB_COUNT 5

int jobs[JOB_COUNT] = {1,2,3,4,5};
int next_job = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int id;
    char name[20];

}Data;

void *worker(void *arg)
{
    Data  *data = (Data *) arg;

    printf("id %d name %s\n",data->id,data->name);

    char *result = malloc(sizeof(17));
    strncpy(result,"kopyalanan islem",17) ;
    
    char *temp = realloc(result,30*sizeof(char));

    result = temp;

    strcat(result, " salih idi");
    
    if (result == NULL)
        return NULL;

    pthread_exit(result);
}

int main()

{
    pthread_t tid;
    void *status;

    Data  data = {42,"Salih"};

    pthread_create(&tid, NULL, worker, &data);
    pthread_join(tid, &status);

    if (status != NULL) {
        printf("%s\n", (char *)status);
        free(status);
    }
    int **x;
    x = malloc(3 * sizeof (*x));// bunla int **x = malloc(3 *sizeof(...)) ayni sey oluyor enterasan sekilde
    for( int i = 0 ; i< 3; i++){
        x[i] = malloc(5* sizeof (int));
    }
    
    int(*y)[2];  // Her satırda 2 int var
    y = malloc(5 * sizeof(*y));  // 5 satır ayır


    return 0;
}
