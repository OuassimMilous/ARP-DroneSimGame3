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

// Global flag to indicate whether the watchdog is running
volatile sig_atomic_t watchdog_running = 1;

// Signal handler for SIGCHLD to track watchdog status
void handle_watchdog_exit(int signum) {
    watchdog_running = 0;
}


int main(int argc, char *argv[]) {

    //declaration of variables
    pid_t server, UI, drone, watchdog, keyboard;
    int res;
    int num_children = 0;


    // PIPES
    int UI_server[2];
    int server_UI[2];

    int keyboard_server[2];
    int server_keyboard[2];

    int drone_server[2];
    int server_drone[2];
   if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

   if (pipe(UI_server)   == -1 ||  pipe(server_UI)   == -1 ||
        pipe(keyboard_server) == -1 ||  pipe(server_keyboard) == -1 ||
        pipe(drone_server)    == -1 ||  pipe(server_drone)    == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    // the semaphore for the log file
    sem_t *semLOG;
    semLOG = sem_open(LOGSEMPATH, O_CREAT, 0666, 1); 
    if (semLOG == SEM_FAILED)
    {
        perror("sem_open1");
        exit(EXIT_FAILURE);
    }
    sem_init(semLOG, 1, 1);


    //cleanup the log file
    sem_wait(semLOG);
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

    sem_post(semLOG);


    // Set up signal handler for child process exit
    signal(SIGCHLD, handle_watchdog_exit);


    // Fork server process
    server = fork();
    if (server < 0) {
        perror("Fork error at server");
        return -1;
    } else if (server == 0) {
       // Child process: Execute server
        char pipeargs [100]; 

        sprintf(pipeargs,"%d %d|%d %d|%d %d|%d %d|%d %d|%d %d",
                                            UI_server[0], UI_server[1],   server_UI[0],   server_UI[1],
                                            keyboard_server[0], keyboard_server[1], server_keyboard[0], server_keyboard[1],
                                            drone_server[0],    drone_server[1],    server_drone[0],    server_drone[1]);
        char *arg_list[] = {argv[1],pipeargs,"&", NULL};
        execvp("./build/server", arg_list);
        
        return 0;  // Should not reach here
    }
    num_children += 1;

    // Fork UI process
    UI = fork();
    if (UI < 0) {
        perror("Fork error at UI");
        return -1;
    } else if (UI == 0) {
        // Child process: Execute UI
        
        char pipeargs [100]; 
        sprintf(pipeargs,"%d %d|%d %d",
                        UI_server[0], UI_server[1],   server_UI[0],   server_UI[1]);

        char *arg_list[] = {"konsole", "-e", "./build/UI",pipeargs, NULL};
        execvp("konsole", arg_list);
        return 0;  // Should not reach here
    }
    num_children += 1;

    // Fork keyboard process
    keyboard = fork();
    if (keyboard < 0) {
        perror("Fork error at keyboard");
        return -1;
    } else if (keyboard == 0) {
        // Child process: Execute keyboard
        char pipeargs [100]; 
        sprintf(pipeargs,"%d %d|%d %d",
                      keyboard_server[0], keyboard_server[1], server_keyboard[0], server_keyboard[1]);

        char *arg_list[] = {"konsole", "-e", "./build/keyboard",pipeargs, NULL};
        execvp("konsole", arg_list);
        return 0;  // Should not reach here
    }
    num_children += 1;

    // Fork drone process
    drone = fork();
    if (drone < 0) {
        perror("Fork error at drone");
        return -1;
    } else if (drone == 0) {
        // Child process: Execute drone

        char pipeargs [100]; 
        sprintf(pipeargs,"%d %d|%d %d",
                        drone_server[0],    drone_server[1],    server_drone[0],    server_drone[1]);
 

        char *arg_list[] = {pipeargs,"&", NULL};
        execvp("./build/drone", arg_list);
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
        char server_str[20], UI_str[20], drone_str[20], keyboard_str[20];
        snprintf(server_str, sizeof(server_str), "%d", server);
        snprintf(UI_str, sizeof(UI_str), "%d", UI);
        snprintf(drone_str, sizeof(drone_str), "%d", drone);
        snprintf(keyboard_str, sizeof(keyboard_str), "%d", keyboard);


        // Build the arg_list with PIDs as command-line arguments
        char *arg_list[] = {"konsole", "-e","./build/watchdog",server_str, UI_str, drone_str, keyboard_str,NULL};
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
    kill(server, SIGTERM);
    kill(UI, SIGTERM);
    kill(drone, SIGTERM);
    kill(keyboard, SIGTERM);
    // Repeat for other processes

    // Wait for all remaining children to terminate
    while (num_children > 0) {
        wait(&res);
        num_children -= 1;
    }

    //clean up
    sem_unlink(LOGSEMPATH); 
    sem_close(semLOG);

    
    // Exit the main process
    return 0;
}

