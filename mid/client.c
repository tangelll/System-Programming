#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <semaphore.h>

#define SHM_NAME "/my_shared_memory"
#define SIZE 1024
#define MAX_CLIENTS 10
#define MSG_SIZE 256

typedef struct {
    pid_t client_pid;
    int status;
    char message[MSG_SIZE];
} ClientSlot;

int fdshared;
int my_slot = -1;
ClientSlot *shared;
volatile sig_atomic_t keep_running = 1;

void signalHandler(int sig) {
    const char msg[] = "Caught SIGINT \n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    keep_running = 0;
    memset(shared,0,SIZE); 
    snprintf(shared[my_slot].message,sizeof(shared[my_slot].message) ,"I am going to die my pid is %d", getpid());

    //temizlik
    munmap(shared, SIZE);
    close(fdshared);
    _exit(sig);
}

int main(){
    signal(SIGINT, signalHandler);
    char message[100]={0};
    char sem_name[64];
    
    snprintf(sem_name, sizeof(sem_name),
             "/client_ready_%d", (int)getpid());

    sem_t *ready_sem = sem_open(
        sem_name, O_CREAT | O_EXCL, 0666, 0
    );

    if (ready_sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }   

    fdshared = shm_open(SHM_NAME, O_RDWR, 0666);

    if (fdshared == -1) {
        perror("shm_open");
        return 1;
    }

    size_t shm_size = sizeof(ClientSlot) * MAX_CLIENTS;
    shared = mmap(NULL, shm_size,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED, fdshared, 0);

    if (shared == MAP_FAILED) {
        perror("mmap");
        close(fdshared);
        exit(EXIT_FAILURE);
    }



	char *fifo_path = "salih_fifo";

	int fd = open(fifo_path,O_WRONLY);

	if( fd == -1){
        	printf("Error message: %s\n", strerror(errno));
		return -1;
	}
	printf("benim pidim bu unutma %d\n",getpid());
	snprintf(message, sizeof(message), "Hello Server I am a Client and My PID is %d", getpid());
	write(fd,message,strlen(message));
	memset(message, 0, sizeof(message));	
    
    int slot_index = -1;
    sem_wait(ready_sem);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (shared[i].client_pid == getpid() ) {
            my_slot = i;
            printf("kendime ait slotu buldum\n");
            break;
        }
    }
    if (my_slot != -1) {
        snprintf(shared[my_slot].message,
                 sizeof(shared[my_slot].message),
                 "bu shared memoryden ilk mesajimdir");
    } else {
        printf("Kendime ait slotu bulamadim!\n");
    }
	sem_close(ready_sem);
    sem_unlink(sem_name);


	while(keep_running){	
        fgets(message, sizeof(message), stdin);
        memset(shared[my_slot].message,0,sizeof(shared[my_slot].message));
        snprintf(shared[my_slot].message,
                 sizeof(shared[my_slot].message),
                 "%s",message);
        shared[my_slot].status = 1;
	}
    munmap(shared, SIZE);
    close(fdshared);
	close(fd);
	return 0;
}

