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


//variables
struct data data;
int targetReached = 0;
char host[50];
int port = 8080;
FILE* file;
sem_t* LOGsem; 
char msg[SOCEKTMSGLEN];
int client_socket;
char buffer[SOCEKTMSGLEN];
char bufferrec[SOCEKTMSGLEN];
char reced [SOCEKTMSGLEN];

// a function to create targets
void createTargets(int num) {
    char tmp[1000];
    for (int i = 0; i < num * 2; i += 2) {
        data.targets[i] = rand() % (int)data.max[0];
        data.targets[i + 1] = rand() % (int)data.max[1];
    }

    sprintf(msg, "T[%d]",num);

    for (int j = 0; j < num * 2; j += 2) {
        sprintf(tmp,"%.3lf,%.3lf", data.targets[j], data.targets[j + 1]);
        strcat(msg, tmp);
        if(j != num * 2 - 2){
           strcat(msg, "|");
        }
    }

    logit(msg);
    writeSocket(msg);
}


// a function to write in the socket
void writeSocket(char *msgs){
        bzero(buffer, SOCEKTMSGLEN);
        strncpy(buffer, msgs, SOCEKTMSGLEN - 1);  
        
        ssize_t bytes_written = write(client_socket, buffer, strlen(buffer));
        if (bytes_written <= 0) {
            perror("matrytawch");
            exit(EXIT_FAILURE);
        }

        sprintf(msg,"[Targets]: socket sent : %s", buffer);
        logit(msg);

        bzero(bufferrec, SOCEKTMSGLEN);
        if (read(client_socket, bufferrec, sizeof(bufferrec) - 1) == -1) {
            perror("writeSocket");
            exit(EXIT_FAILURE);
        }

        sprintf(msg,"[Targets]: socket echo: %s", bufferrec);
        logit(msg);

        usleep(SOCKETWAIT);

    // leave if the echo is wrong
    if(strncmp(buffer, bufferrec, SOCEKTMSGLEN - 1)){
        exit(1);
    }
    
}


// a function to read from the socket
void readSocket() {

   // Read the message from the client
    bzero(buffer, SOCEKTMSGLEN);
    if (read(client_socket, buffer, sizeof(buffer) - 1) == -1) {
        perror("readSocket");
        exit(EXIT_FAILURE);
    }

    strncpy(reced, buffer, SOCEKTMSGLEN - 1);  

    sprintf(msg,"[Targets]: socket recieved: %s", buffer);
    logit(msg);
    // Echo the message back to the client

    bzero(bufferrec, SOCEKTMSGLEN);
    strncpy(bufferrec, buffer, SOCEKTMSGLEN - 1);  
    ssize_t bytes_written = write(client_socket, bufferrec, strlen(bufferrec));
    if (bytes_written <= 0) {
        perror("matrytawch< ta3");
        exit(EXIT_FAILURE);
    }

    sprintf(msg,"[Targets]:Sent echo back to server: %s", bufferrec);
    logit(msg);

    usleep(SOCKETWAIT);
    // leave if the echo is wrong
    if(strncmp(buffer, bufferrec, SOCEKTMSGLEN - 1)){
        exit(1);
    }
}



// a funciton to log
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
      // open the semaphore for logging
    LOGsem = sem_open(LOGSEMPATH, O_RDWR, 0666); 
    if (LOGsem == SEM_FAILED) {
        perror("sem_open" );
        exit(EXIT_FAILURE);
    }

    sprintf(msg,"[Targets]: ALIVE");
    logit(msg);

    // extract the port
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
    writeSocket("TI");
    // recieve the dimentions
    readSocket();
    logit(reced);
    sscanf(reced, "%lf,%lf", &data.max[0], &data.max[1]);

    //create the targets
    createTargets(NUM_TARGETS);


    while(1) {
        // recieve data
        readSocket();
        if (strcmp(reced, "GE") == 0) {
            // create targets again
            createTargets(NUM_TARGETS);
            sprintf(reced, "0");
        } else if (strcmp(reced, "stop") == 0) {
            // exit program
            exit(0);
        }

    }
}