#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>

#define SHM_NAME "/my_shared_memory"
#define SIZE 1024
int fdshared;
char *shared;

volatile sig_atomic_t keep_running = 1;

void signalHandler(int sig) {
    const char msg[] = "Caught SIGINT\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    keep_running = 0;
    memset(shared,0,SIZE); 
    snprintf(shared, SIZE, "I am going to die my pid is %d", getpid());

    //temizlik
    munmap(shared, SIZE);
    close(fdshared);
    _exit(sig);
}

int main(){
    signal(SIGINT, signalHandler); 
    fdshared = shm_open(SHM_NAME, O_RDWR, 0666);
    char message[100]={0};

    if (fdshared == -1) {
        perror("shm_open");
        return 1;
    }

    // Memory'yi kendi adres alanına bağla
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

    
	char *fifo_path = "salih_fifo";

	int fd = open(fifo_path,O_WRONLY);

	if( fd == -1){
        	printf("Error message: %s\n", strerror(errno));
		return -1;
	}
	
	snprintf(message, sizeof(message), "Hello Server I am a Client and My PID is %d", getpid());
	write(fd,message,strlen(message));
	
	memset(message, 0, sizeof(message));	
	
	while(keep_running){	
	//scanf("%99s",message);
	    sleep(1);
	}
        //munmap(shared, SIZE);
        //close(fdshared);
	//close(fd);
	return 0;
}

