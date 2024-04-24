#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "ADTQueue.h"
#include "ADTList.h"

#define READ 0
#define WRITE 1
#define BUFFSIZE 16384

char* get_pipeName(pid_t workerId);

void listener(char* args[]);

char* get_filename(char* buffer);

int* create_int(int value) {
	int* p = malloc(sizeof(int));
	*p = value;
	return p;
}

/* Global variables for sigint handler */
int end = 1;

/* SIGINT Handler */
void sig_int(int signo){
    pid_t stoppedWorker;
    int status;

    end = 0;
    
    /* Find every stopped process and kill it */ 
    while((stoppedWorker = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0){
        kill(SIGKILL, stoppedWorker);
    }
}

/* Global variable for sigchld handler */
int workerAvailable = 0;

/* SIGCHLD Handler */
void sig_chld(int signo){
    signal(SIGCHLD, sig_chld);
    workerAvailable = 1;
}

int main(int argc, char* argv[]){
    /* Signals */
    static struct sigaction act;
    act.sa_handler = sig_chld;
    act.sa_flags = 0;
    act.sa_flags |= SA_RESTART;
    sigaction(SIGCHLD, &act, NULL);

    static struct sigaction act_int;
    act_int.sa_handler = sig_int;
    sigaction(SIGINT, &act_int, NULL);

    /* Get path to monitor */
    char* mpath;
    if(argc > 1){
        mpath = argv[2];
    }
    else{
        mpath = ".";
    }

    int fd[2];

    /* Create the pipe */
    if(pipe(fd) == -1){
        perror("pipe call");
        exit(1);
    }
    /* Create child process */
    pid_t pid = fork();
    if(pid < 0){
        perror("fork call");
        exit(2);
    }
    /* Listener */
    if(pid == 0){
        dup2(fd[WRITE], STDOUT_FILENO);
        close(fd[READ]); // listener is writing

        char* args[] = {"inotifywait", mpath, "-m", "-e", "create", "-e", "moved_to", NULL};
        listener(args);
    }

    /* Priority Queue for workers */
    Queue q = q_create(free);
    pid_t pq_pid;

    int readfd, writefd, md;

    /* Manager is reading */
    char buffer[BUFFSIZE];
    pid_t pid2, workerId=0, stoppedWorker;
    char* pipeName;
    char* pipeNumber, *buf;
    int* max, length, status, *pip;
    while(end){
        if(workerAvailable){
            /* Get stopped processes */
            while((stoppedWorker = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0){
                q_insert(q, create_int(stoppedWorker));
            }
        }
        ssize_t bytes = read(fd[READ], buffer, BUFFSIZE);
        if(bytes > 0){
            /* Get the filename */
            buf = strndup(buffer, bytes);
            printf("\n\n%s", buf);
            
            char* filename = get_filename(buf);
            
            /* If there are no available workers we fork */
            if(q_size(q) == 0){
                pid2 = fork();
                stoppedWorker = 0;
                /* Create Worker */
                if(pid2 < 0){
                    perror("fork call");
                    exit(2);
                }
                else if(pid2 != 0){
                    /* Create named pipe */
                    workerId = pid2;
                    pipeName = get_pipeName(workerId);
                    printf("Created named pipe %s \n", pipeName);
                    if((mkfifo(pipeName, 0666) < 0) && (errno != EEXIST)){
                        perror("Cant create fifo");
                    }
                    /* Parent writes in pipe */
                    printf("Manager writing in pipe %s \n", pipeName);
                    if((writefd = open(pipeName, O_WRONLY)) < 0){
                        perror("Manager cant open write fifo");
                        exit(1);
                    }
                    if(write(writefd, filename, strlen(filename)) != strlen(filename)){
                        perror("write error");
                        exit(4);
                    }
                    free(pipeName);
                    close(writefd);
                }
                if(pid2 == 0){
                    /* Get named pipe name */
                    workerId = getpid();
                    pipeName = get_pipeName(workerId);
                    
                    printf("Calling worker with pipeName : %s \n", pipeName);
                    char* args[] = {"./worker", pipeName};
                    execvp(args[0], args);
                }
            }
            /* If there are available workers in queue we SIGCONT the first one and remove
            him from queue */
            else{
                /* Get named pipe name */
                pip = q_first(q);
                workerId =  *pip;
                pipeName = get_pipeName(workerId);
                printf("Child to be continued is %d \n", workerId);

                /* Continue stopped worker */
                kill(workerId, SIGCONT);
                
                /* Parent writes in pipe */
                printf("Second manager writing in pipe %s \n", pipeName);
                writefd = (open(pipeName, O_WRONLY));
                if((writefd = (open(pipeName, O_WRONLY))) < 0){
                    perror("Manager cant open write fifo");
                    exit(1);
                }
                if(write(writefd, filename, strlen(filename)) != strlen(filename)){
                    perror("Manager write error");
                    exit(4);
                }
                
                close(writefd);
                
                /* Remove it from queue */
                q_remove_first(q);
                free(pipeName);
            }
            free(filename);
            free(buf);
        }
        else if(bytes == 0){
            break;
        }
    }
    /* Free Memory */
    q_destroy(q);

    return 1;
}

char* get_filename(char* buffer){
    int counter = 0;
    char* token, *filename;
    char* p = malloc(strlen(buffer)+1);
    strcpy(p, buffer);

    token = strtok(p, " ");
    char* path = malloc((strlen(token)+1));
    strcpy(path, token);
    while(token != NULL){
        token = strtok(NULL, " ");
        counter++;
        if(counter == 2){
            filename = malloc(strlen(token)+strlen(path)+2);
            strcpy(filename, path);
            strcat(filename, token);
        }
    }
    /* inotifywait output is like "./ CREATE <filename>\n" so i remove the \n*/
    filename[strlen(filename)-1] = '\0';

    char* newLine;
    int index;
    if((newLine = strchr(filename, '\n')) != NULL){
        index = (int)(newLine - filename);
        filename[index] = '\0';
    }
    free(p); free(path);

    return filename;
}

char* get_pipeName(pid_t workerId){
    int length;
    char* pipeName, *pipeNumber;

    /* Get length of worker id number */
    length = snprintf(NULL, 0, "%d", workerId) + 1;
    pipeNumber = malloc(length);
    snprintf(pipeNumber, length+1, "%d", workerId);
    /* Name will be of type "/tmp/fifo.(workerId)" */
    pipeName = malloc(length+13);
    strcpy(pipeName, "/tmp/fifo.");
    strcat(pipeName, pipeNumber);

    free(pipeNumber);

    return pipeName;
}