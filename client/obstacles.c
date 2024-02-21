#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <ncurses.h>
#include <time.h>
#include "include/constants.h"
#include <math.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <errno.h>
#include <sys/select.h>
#include <time.h>

//variables
struct data data;
int targetReached = 0;

char host[50];
int port = 8080;
FILE* file;
sem_t* LOGsem; 
char msg[100];
int client_socket;
char buffer[SOCEKTMSGLEN];
char bufferrec[SOCEKTMSGLEN];
char reced [SOCEKTMSGLEN];


// a function to write into the socket
void writeSocket(char *msgs){
        bzero(buffer, SOCEKTMSGLEN);
        strncpy(buffer, msgs, SOCEKTMSGLEN - 1);  
        
        ssize_t bytes_written = write(client_socket, buffer, strlen(buffer));
        if (bytes_written <= 0) {
            perror("matrytawch");
            exit(EXIT_FAILURE);
        }

        sprintf(msg,"[Obstacle]: socket sent : %s", buffer);
        logit(msg);

        bzero(bufferrec, SOCEKTMSGLEN);
        if (read(client_socket, bufferrec, sizeof(bufferrec) - 1) == -1) {
            perror("writeSocket");
            exit(EXIT_FAILURE);
        }

        sprintf(msg,"[Obstacle]: socket echo: %s", bufferrec);
        logit(msg);

        usleep(SOCKETWAIT);
    // leave if the echo is wrong
    if(strncmp(buffer, bufferrec, SOCEKTMSGLEN - 1)){
        exit(1);
    }
}


// a funciton to read from the socket
void readSocket() {

   // Read the message from the client
    bzero(buffer, SOCEKTMSGLEN);
    if (read(client_socket, buffer, sizeof(buffer) - 1) == -1) {
        perror("readSocket");
        exit(EXIT_FAILURE);
    }

    strncpy(reced, buffer, SOCEKTMSGLEN - 1);  

    sprintf(msg,"[Obstacle]: socket recieved: %s", buffer);
    logit(msg);
    // Echo the message back to the client

    bzero(bufferrec, SOCEKTMSGLEN);
    strncpy(bufferrec, buffer, SOCEKTMSGLEN - 1);  
    // sem_wait(sem);
    // write(client_socket, bufferrec, strlen(bufferrec));
    ssize_t bytes_written = write(client_socket, bufferrec, strlen(bufferrec));
    if (bytes_written <= 0) {
        perror("matrytawch< ta3");
        exit(EXIT_FAILURE);
    }

    sprintf(msg,"[Obstacle]:Sent echo back to server: %s", bufferrec);
    logit(msg);

    usleep(SOCKETWAIT);

    // leave if the echo is wrong
    if(strncmp(buffer, bufferrec, SOCEKTMSGLEN - 1)){
        exit(1);
    }
}


// a function to create the obsticales
void createObstecales(int num){
      char tmp[1000];
       // Create obstacles
        for (int i = 0; i < NUM_OBSTACLES * 2; i += 2) {
            data.obstacles[i + 1] = rand() % (int)data.max[1];
             data.obstacles[i] = rand() % (int)data.max[0];
        }


    sprintf(msg, "O[%d]",num);

    for (int j = 0; j < num * 2; j += 2) {
        sprintf
        (tmp,"%.3lf,%.3lf", data.obstacles[j], data.obstacles[j + 1]);
        strcat(msg, tmp);
        if(j != num * 2 - 2){
           strcat(msg, "|");
        }
    }

    logit(msg);
    writeSocket(msg);
}

// a function to save logs
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

int main(int argc, char *argv[])
{
      //open the semaphore for logging
    LOGsem = sem_open(LOGSEMPATH, O_RDWR, 0666); 
    if (LOGsem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    sprintf(msg,"[Obstacles]: ALIVE");
    logit(msg);

    sscanf(argv[0], "%d", &port);
    strncpy(host, argv[1], sizeof(host) - 1);

    // Create socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(host);
    server_addr.sin_port = htons(port);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("ERROR connecting");
        exit(EXIT_FAILURE);
    }

    // initialize
    writeSocket("OI");

    // read dimentions
    readSocket();
    logit(reced);
    sscanf(reced, "%lf,%lf", &data.max[0], &data.max[1]);
    // create the first obstacles
    createObstecales(NUM_OBSTACLES);
    
    int i = 0;
    while(1) {
    // wait for a while then change obstacles
    if (i > 7 * 10 ) {
        createObstecales(NUM_OBSTACLES);
        i = 0;  // Reset the counter
    } else {
        usleep(1);
        i++;
    }

    // recive the data from the socket in a non blocking way
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(client_socket, &read_fds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;  // Set timeout to 0.1 seconds (adjust as needed)

    int ready_fds = select(client_socket + 1, &read_fds, NULL, NULL, &timeout);

    if (ready_fds == -1) {
        perror("select");
        exit(EXIT_FAILURE);
    } else if (ready_fds == 0) {
        // Timeout occurred, handle it if needed
    } else {
        // Perform read only if the socket is ready
        if (FD_ISSET(client_socket, &read_fds)) {
            readSocket();
        }  
    }
        // exit
        if (strcmp(reced, "stop") == 0) {
            exit(0);
        }

    }
}