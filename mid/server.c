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
#include <semaphore.h>
#include <fcntl.h>

volatile sig_atomic_t keep_running = 1;
#define SHM_NAME "/my_shared_memory"
#define SIZE 1024
#define MAX_CLIENTS 10
#define MSG_SIZE 256

typedef struct {
    pid_t client_pid;
    int status;
    char message[MSG_SIZE];
} ClientSlot;

ClientSlot *shared;
int fd;
int fdshared;
pid_t (*client_pids)[2] = NULL;
int client_count = 0;
int server_pid = -1;
int slot_index = -1;

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
    if ( getpid() == server_pid)
    {
        const char msg[] = "Caught SIGINT\n";
        write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    }
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

void handle_read_file(char *filename, pid_t clientpid ) {
    
    //semaphorlar
    char sem_write_name[64], sem_read_name[64];
    snprintf(sem_write_name, sizeof(sem_write_name), "/sem_write_%d", (int)clientpid);
    snprintf(sem_read_name, sizeof(sem_read_name), "/sem_read_%d", (int)clientpid);

    sem_t *sem_write = sem_open(sem_write_name, 0);
    sem_t *sem_read = sem_open(sem_read_name, 0);

    int fd = open(filename, O_RDONLY);

    if (fd == -1) {

        perror("Dosya acilamadi");

        sem_wait(sem_write);

    memset(shared[slot_index].message, 0,
           sizeof(shared[slot_index].message));

    snprintf(shared[slot_index].message,
             sizeof(shared[slot_index].message),
             "%s", "ERROR: Dosya bulunamadi veya acilamadi.");

    sem_post(sem_read);
    sem_wait(sem_write);


        sem_close(sem_write);
        sem_close(sem_read);
        return;
    }

    struct flock lock;
    memset(&lock, 0, sizeof(lock));

    lock.l_type = F_RDLCK;    // F_RDLCK: Okuma kilidi (Birden fazla süreç aynı anda okuyabilir ama kimse yazamaz)
    lock.l_whence = SEEK_SET; // Kilidin dosyanın başından başlayacağını belirtir
    lock.l_start = 0;         // 0. bayttan başla
    lock.l_len = 0;           // 0 demek "Tüm dosyayı kilitle" demektir


    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("Kilitlenirken hata olustu");
        close(fd);
        sem_close(sem_write);
        sem_close(sem_read);

        return;
    }
    
    

    // 4. Dosyayı okuma işlemi
    char buffer[256];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0'; 
       
        sem_wait(sem_write);



        memset(shared[slot_index].message, 0, sizeof(shared[slot_index].message));
        snprintf(shared[slot_index].message, sizeof(shared[slot_index].message), "%s", buffer);
        shared[slot_index].status = 1;


        sem_post(sem_read); 

    }

    sem_wait(sem_write);
    memset(shared[slot_index].message, 0, sizeof(shared[slot_index].message));
    snprintf(shared[slot_index].message, sizeof(shared[slot_index].message), "EOF");
    sem_post(sem_read); 
    sem_wait(sem_write);

    sem_close(sem_write);
    sem_close(sem_read);

    lock.l_type = F_UNLCK; // F_UNLCK: Kilidi açma modu
    fcntl(fd, F_SETLK, &lock); // Kilidi kaldırıyoruz

    close(fd); 
}

int main(){
	char *fifo_path = "salih_fifo";
	char buffer [100];
	int result = mkfifo(fifo_path,0666);
    signal(SIGINT, signalHandler); 
    signal(SIGCHLD, child_handler);
    server_pid = getpid();

    fdshared = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fdshared == -1) {
 
        perror("shm_open");
         return 1;
    }

    size_t shm_size = sizeof(ClientSlot) * MAX_CLIENTS;

    ftruncate(fdshared, shm_size);
    if (fdshared == -1) {
        perror("shm_open");
        return 1;
    }


    shared = mmap(
        NULL,
        shm_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fdshared,
        0
    );
    if (shared == MAP_FAILED) {
        perror("mmap");
        return 1;
    }      

    

	printf("Waiting for a reader to connect...\n");

	fd = open(fifo_path,O_RDONLY);

	if( fd == -1){
        	printf("Error message: %s\n", strerror(errno));
		return -1;
	}
	
	while(keep_running){
		int client_alive =1 ;
		int n = read(fd,buffer,100);
		if (strlen(buffer) != 0)
		{
			buffer[n] = '\0';
			int clientpid; 
			
			//gelen process yeni mi yoksa sistemde var mi kontrol ediyoruz.
			if(0 == strncmp("Hello Server I am a Client",buffer,26)){
				sscanf(buffer, "%*[^0-9]%d", &clientpid);
                slot_index = -1;

                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (shared[i].client_pid == 0) {
                        slot_index = i;
                        break;
                    }
                }

                if (slot_index == -1) {
                    printf("Boş client slotu yok.\n");
                } else {
                    shared[slot_index].client_pid = clientpid;
                    shared[slot_index].status = 0;

                    printf("slot=%d, PID=%d\n",
       slot_index, shared[slot_index].client_pid);
                    char sem_name[64];
                    snprintf(sem_name, sizeof(sem_name),
                     "/client_ready_%d", (int)clientpid);

                    sem_t *ready_sem = sem_open(sem_name, 0);

                    if (ready_sem == SEM_FAILED) {
                        perror("sem_open");
                    } else {
                        sem_post(ready_sem);
                        sem_close(ready_sem);
                    }
                }
				
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
                {
				    while(client_alive){
                        pid_t mypid;
                        if (sscanf(shared[slot_index].message, "I am going to die my pid is %d", &mypid) == 1) {
                            if (mypid == clientpid) {
                            printf("Benim client kapanıyor: %d\n", clientpid);
                            client_alive = 0;
                            }
                        }
                        else{
                            if(shared[slot_index].status == 1)
                            {
                                char filename[100];
                                if( sscanf(shared[slot_index].message, "readF %99s", filename) == 1 )
                                {
                                    handle_read_file(filename,clientpid);

                                }else{
                                
                                printf("%s\n",shared[slot_index].message);
                                memset(shared[slot_index].message,0,sizeof(shared[slot_index].message));
                                shared[slot_index].status = 0;
                                }
                            }
                        }
                    
                    }
                        puts("Child exiting.");
                         fflush(stdout);
                        _exit(EXIT_SUCCESS);
               }
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


