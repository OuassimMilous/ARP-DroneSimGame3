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



struct data data;
int targetReached = 0;

char host[50];
int port = 8080;
FILE* file;
sem_t* LOGsem; 
char msg[100];
int client_socket;
// int server_socket;
char buffer[SOCEKTMSGLEN];
char bufferrec[SOCEKTMSGLEN];

char reced [SOCEKTMSGLEN];



void writeSocket(char *msgs){
    do{
        bzero(buffer, SOCEKTMSGLEN);
        strncpy(buffer, msgs, SOCEKTMSGLEN - 1);  
        // write(client_socket, buffer, strlen(buffer));
        
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

    } while (strncmp(buffer, bufferrec, SOCEKTMSGLEN - 1) != 0);
    
    }



void readSocket() {
 do
 {
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
} while (strncmp(buffer, bufferrec, SOCEKTMSGLEN - 1) != 0);

}



void createObstecales(int num){
      char tmp[1000];

       // Create obstacles
        for (int i = 0; i < NUM_OBSTACLES * 2; i += 2) {
            data.obstacles[i] = rand() % (int)data.max[0];
            data.obstacles[i + 1] = rand() % (int)data.max[1];
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
      // Create or open a semaphore for logging
    LOGsem = sem_open(LOGSEMPATH, O_RDWR, 0666); 
    if (LOGsem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }



    sprintf(msg,"[Obstacles]: ALIVE");
    logit(msg);

  //  read(server_obstacles[0], &data, sizeof(data));
    // sscanf(argv[0], "%d", &client_socket);
    // sscanf(argv[0], "%d %s", &host,&port);
    // sscanf(argv[1], "%d",&port);
    sscanf(argv[0], "%d", &port);
    strncpy(host, argv[1], sizeof(host) - 1);

    // Create socket
    //client_socket;
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


    writeSocket("OI");

    readSocket();
    logit(reced);
    // sscanf(reced, "%.3f,%.3f", &data.max[0], &data.max[1]);
    sscanf(reced, "%lf,%lf", &data.max[0], &data.max[1]);
    sprintf(msg, "TEXTAT YAW  %.3f,%.3f ", &data.max[0], &data.max[1]);
 
    //  sprintf(msg, "TEXTAT YAW  %.3lf,%.3lf", data.max[0], data.max[1]);
    //     logit(msg);

    // sscanf(msg, "%.3f,%.3f", &data.max[0], &data.max[1]);
  
    // createObstecales(NUM_OBSTACLES);


    while(1) {
            //create targets
          createObstecales(NUM_OBSTACLES);
          sleep(6);

        
    }
    
//     while(1) {
//   // //  sprintf(dest_buffer, "%f.3", double_or_float_number);
//   //     logit("[Obstacles]: sending");
//   // writeSocket("obs");
  
//   //   usleep(50000);


//        // Create obstacles
// //         for (int i = 0; i < NUM_OBSTACLES * 2; i += 2) {
// //             do {
// //                 data.obstacles[i] = rand() % (int)data.max[0];
// //             } while (fabs(data.obstacles[i] - data.targets[i]) < THRESH_TARGET || fabs(data.obstacles[i] - data.drone_pos[0]) < THRESH_TARGET);

// //             do {
// //                 data.obstacles[i + 1] = rand() % (int)data.max[1];
// //             } while (fabs(data.obstacles[i + 1] - data.targets[i + 1]) < THRESH_TARGET || fabs(data.obstacles[i + 1] - data.drone_pos[1]) < THRESH_TARGET);

// //             sprintf(msg, "[Obstacles]: x[%d]: %f, y[%d]: %f", i/2, data.obstacles[i], i/2, data.obstacles[i + 1]);
// //             logit(msg);
// //         }
// //     //    write(obstacles_server[1], &data, sizeof(data));
// //         sleep(10);
// //    //     read(server_obstacles[0], &data, sizeof(data)); //update dronepos
//     }

}