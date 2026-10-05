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
pid_t (*client_pids)[2] = NULL;
int client_count = 0;

void child_handler(int sig)
{
    int status;
    pid_t pid;
while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {

    printf("Child %d öldü.\n", pid);

    for (int i = 0; i < client_count; i++) {
        if (pid == client_pids[i][1]) {

            client_pids[i][0] = 0;
            client_pids[i][1] = 0;

            printf("%d nolu child listeden silindi\n", pid);
            break;
        }
    }
}
}

void printList(pid_t (*client_pids)[2],int size){
	for(int i = 0 ; i < size; i++){
		printf("musteri %d clientpid: %d yavrusu: %d\n",i,client_pids[i][0],client_pids[i][1]);
	}
}
void signalHandler(int sig) {
    const char msg[] = "Caught SIGINT\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    keep_running = 0;

for (int i = 0; i < client_count; i++)
    kill(client_pids[i][0], SIGTERM);

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
        signal(SIGCHLD, child_handler);


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
		int		client_alive =1 ;
		int n = read(fd,buffer,100);
		if (strlen(buffer) != 0)
		{
			buffer[n] = '\0';
			int clientpid; 
			
			//gelen process yeni mi yoksa sistemde var mi kontrol ediyoruz.
			if(0 == strncmp("Hello Server I am a Client",buffer,26)){
				
				sscanf(buffer, "%*[^0-9]%d", &clientpid);
				
				//process olusturma	
				pid_t childpid;

				if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
				perror("signal");
			        exit(EXIT_FAILURE);
			   	}
				   childpid = fork();
				   switch (childpid) {
				   case -1:
				       perror("fork");
			      	 exit(EXIT_FAILURE);
			   	case 0:
				while(client_alive){
					pid_t mypid;
					if (sscanf(shared, "I am going to die my pid is %d", &mypid) == 1) {
					    if (mypid == clientpid) {
						printf("Benim client kapanıyor: %d\n", clientpid);
						client_alive = 0;
					    }
					}
				
				}
			       	puts("Child exiting.");
			      	 fflush(stdout);
			       	_exit(EXIT_SUCCESS);
			   	default:
			       	printf("Child is PID %jd\n", (intmax_t) childpid);
				printf("Client is  PID:%d\n", clientpid);

				client_pids = realloc(client_pids,(client_count + 1) * sizeof(*client_pids));
					
				client_pids[client_count][0] = clientpid;
				client_pids[client_count][1] = childpid;
				client_count++;
		//		printList(client_pids,client_count);
			
				}
			}else{
				// bisi ypamaya gerek yok sadece ilk kez fifoya yaziyor
			}
			
		}
		buffer[0] = '\0';
	
	}
    	close(fd);

}


