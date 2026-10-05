#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdbool.h>

volatile sig_atomic_t keep_running = 1;
#define SHM_NAME "/my_shared_memory"
#define SIZE 1024
int fd;
int fdshared;
char *shared;
pid_t *client_pids = NULL;
int client_count = 0;

bool isInList(int pid, int client_pids[],int size){
	for(int i = 0 ; i < size; i++){
		if(pid == client_pids[i]){ return true;}
	}
	return false;
}
void signalHandler(int sig) {
    const char msg[] = "Caught SIGINT\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    keep_running = 0;

for (int i = 0; i < client_count; i++)
    kill(client_pids[i], SIGTERM);

free(client_pids);
    // Temizlik
    munmap(shared, SIZE);
    close(fdshared);
    shm_unlink(SHM_NAME);
    _exit(sig);
}
int main(){
	char *fifo_path = "salih_fifo";
	char buffer [100];
	int result = mkfifo(fifo_path,0666);
        signal(SIGINT, signalHandler); 

	fdshared = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);

    if (fdshared == -1) {
        perror("shm_open");
        return 1;
    }

    // Shared memory'nin boyutunu belirle
    ftruncate(fdshared, SIZE);

    // Memory'yi process'in adres alanına bağla
    shared = mmap(
        NULL,
        SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fdshared,
        0
    );

    if (shared == MAP_FAILED) {
        perror("mmap");
        return 1;
    }



	
#if 0
	if( result == -1){
        	printf("Error message: %s\n", strerror(errno));
		return -1;
	}
#endif

	printf("Waiting for a reader to connect...\n");

	fd = open(fifo_path,O_RDONLY);

	if( fd == -1){
        	printf("Error message: %s\n", strerror(errno));
		return -1;
	}
	
	while(keep_running){
		int n = read(fd,buffer,100);
		if (strlen(buffer) != 0)
		{
			buffer[n] = '\0';
			int pid; 
			sscanf(buffer, "%*[^0-9]%d", &pid);
			
			//gelen process yeni mi yoksa sistemde var mi kontrol ediyoruz.
			if(isInList(pid,client_pids,client_count) && strncmp("Hello Server I am a Client",buffer,26) ){
				printf("Message: %s\n",buffer);
			}
			else
			{
				printf("PID:%d\n", pid);
				client_pids = realloc(client_pids,(client_count + 1) * sizeof(pid_t));
				client_pids[client_count] = pid;
				client_count++;
			
			}
			
		}
		buffer[0] = '\0';
	
	}
    	close(fd);

}


