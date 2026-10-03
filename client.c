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

int main(){
    int fdshared = shm_open(SHM_NAME, O_RDWR, 0666);
    char message[100]={0};

    if (fdshared == -1) {
        perror("shm_open");
        return 1;
    }

    // Memory'yi kendi adres alanına bağla
    char *shared = mmap(
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
	
	scanf("%99s",message);
        munmap(shared, SIZE);
        close(fdshared);

	close(fd);
	return 0;
}

