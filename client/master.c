#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/select.h>
#include <unistd.h> 
#include <stdlib.h>
#include "include/constants.h"
#include <semaphore.h>
#include <sys/mman.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>

// Global flag to indicate whether the watchdog is running
volatile sig_atomic_t watchdog_running = 1;

// Signal handler for SIGCHLD to track watchdog status
void handle_watchdog_exit(int signum) {
    watchdog_running = 0;
}

//declare variables
int port;
FILE* file;
sem_t* LOGsem, *sem; 
char msg[100];

//the  log fucntion
void logit(char *msg){
      sem_wait(LOGsem);

        file = fopen( LOGPATH, "a");
        // Check if the file was opened successfully
        if (file == NULL) {
            fprintf(stderr, "Error opening the file.\n");
            exit(EXIT_FAILURE);
        }

        // Write the string to the file
        fprintf(file, "%s\n", msg);
        // Close the file
        fclose(file);
        sem_post(LOGsem);
}

int main(int argc, char *argv[]) {

    //checking for port ad host
   if (argc < 3) {
         fprintf(stderr,"ERROR, no host/port provided\n");
         exit(1);
     }

    // parsing the port and host
    char host[50];
    strncpy(host, argv[1], sizeof(host) - 1);
    int port = atoi(argv[2]);

    //declaration of variables
    pid_t watchdog, obstacles,targets;
    int res;
    int num_children = 0;
    
    // the semaphore for the log file
    LOGsem = sem_open(LOGSEMPATH, O_CREAT, 0666, 1); 
    if (LOGsem == SEM_FAILED)
    {
        perror("sem_open1");
        exit(EXIT_FAILURE);
    }
    sem_init(LOGsem, 1, 1);


    //cleanup the log file
    sem_wait(LOGsem);
    // Open the file in write mode ("w" stands for write)
    FILE *file = fopen(LOGPATH, "w");
    // Check if the file was opened successfully
    if (file == NULL) {
        fprintf(stderr, "Error opening the file.\n");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "--------------------------------------------\n");
    // Close the file
    fclose(file);

    sem_post(LOGsem);

    // Set up signal handler for child process exit
    signal(SIGCHLD, handle_watchdog_exit);
    
    // Fork targets process    
    targets = fork();
    if (targets < 0) {
        perror("Fork error at targets");
        return -1;
    } else if(targets == 0) {
        // Child process: Execute targets

        char pipeargs [100]; 
        sprintf(pipeargs,"%d",
                port);
        char *arg_list[] = {pipeargs,host,"&", NULL};
        execvp("./build/targets", arg_list);
        return 0;  // Should not reach here
    }
    num_children += 1;
    
    // Fork obstacles process
    obstacles = fork();
    if (obstacles < 0) {
        perror("Fork error at obstacles");
        return -1;
    } else if(obstacles == 0) {
        // Child process: Execute obstacles
         char pipeargs [100]; 
        sprintf(pipeargs,"%d",
                port);
        char *arg_list[] = {pipeargs,host,"&", NULL};
        execvp("./build/obstacles", arg_list);
        return 0;  // Should not reach here
    }
    num_children += 1;
    
    // Fork watchdog process
    watchdog = fork();
    if (watchdog < 0) {
        perror("Fork error at watchdog");
        return -1;
    } else if (watchdog == 0) {
        // Child process: Execute watchdog

        // Convert PIDs to strings
        char obstacles_str[20],targets_str[20];
        snprintf(targets_str, sizeof(targets_str), "%d", targets);
        snprintf(obstacles_str, sizeof(obstacles_str), "%d", obstacles);

        // Build the arg_list with PIDs as command-line arguments
        char *arg_list[] = {"konsole", "-e","./build/watchdog",obstacles_str,targets_str,NULL};
        execvp("konsole", arg_list);
        return 0;  // Should not reach here
    }
    num_children += 1;

    // Wait for all children to terminate or for the watchdog to exit
    while (num_children > 0 && watchdog_running) {
        pid_t child_pid = wait(&res);
        if (child_pid == watchdog) {
            // Watchdog process has exited, set the flag to terminate
            watchdog_running = 0;
        }
        num_children -= 1;
    }

    // Send signals to terminate the remaining child processes
    kill(targets, SIGTERM);
    kill(obstacles, SIGTERM);

    // Wait for all remaining children to terminate
    while (num_children > 0) {
        wait(&res);
        num_children -= 1;
    }

    //clean up
    sem_unlink(LOGSEMPATH); 
    sem_close(LOGsem);

    // Exit the main process
    return 0;
}

