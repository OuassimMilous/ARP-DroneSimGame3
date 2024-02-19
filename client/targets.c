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

    // char* host = "127.0.0.1";
    char host[50];
    int port = 8080;
FILE* file;
sem_t* LOGsem; 
char msg[100];
int client_socket;
char buffer[SOCEKTMSGLEN];
char bufferrec[SOCEKTMSGLEN];
char reced [SOCEKTMSGLEN];

// int writeSocket(char *msg){

//     bzero(buffer, SOCEKTMSGLEN);
//     strncpy(buffer, msg, SOCEKTMSGLEN - 1);  
//     sem_wait(sem);
//     write(client_socket, buffer, strlen(buffer));
//     bzero(bufferrec, SOCEKTMSGLEN);
//     read(client_socket, bufferrec, SOCEKTMSGLEN - 1);
//     sem_post(sem);

//     sprintf(msg, "[targets]: socket echo : %s", bufferrec);
//     logit(msg);

//     return strcmp(bufferrec,buffer);
// }


// void createTargets(int num) {
//     for (int i = 0; i < num * 2; i += 2) {
//         // do {
//             data.targets[i] = rand() % (int)data.max[0];
//         // } while (fabs(data.targets[i] - data.drone_pos[0]) < THRESH_TARGET);
// //drone pos mch mdeclarya
//         // do {
//             data.targets[i + 1] = rand() % (int)data.max[1];
//         // } while (fabs(data.targets[i + 1] - data.drone_pos[1]) < THRESH_TARGET);
//     }

//     int index = 0;

//     // Add the 'T[' part to the formatted string
//     index += sprintf(msg, "T[");

//     for (int j = 0; j < num - 1; j += 2) {
//         // Add x and y coordinates separated by a comma
//         index += sprintf(msg + index, "%.3lf,%.3lf", data.targets[j], data.targets[j + 1]);

//         // If there are more targets, add a pipe to separate obstacles
//         if (j + 2 < num) {
//             index += sprintf(msg + index, "|");
//         }
//     }

//     // Add the closing bracket and null terminator
//     index += sprintf(msg + index, "]");

//   logit(msg);
//    writeSocket(msg);        
// }


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

    sprintf(msg,"[Targets]: socket recieved: %s", buffer);
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

    sprintf(msg,"[Targets]:Sent echo back to server: %s", bufferrec);
    logit(msg);

    usleep(SOCKETWAIT);
} while (strncmp(buffer, bufferrec, SOCEKTMSGLEN - 1) != 0);

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
        perror("sem_open" );
        exit(EXIT_FAILURE);
    }




    sprintf(msg,"[Targets]: ALIVE");
    logit(msg);


   // read(server_targets[0], &data, sizeof(data));
    sscanf(argv[0], "%d", &port);
    strncpy(host, argv[1], sizeof(host) - 1);

    // sscanf(argv[0], "%d %s", &host,&port);

    // Create socket
  //  client_socket;
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

    readSocket();
    logit(reced);
    // sscanf(reced, "%.3f,%.3f", &data.max[0], &data.max[1]);
    sscanf(reced, "%lf,%lf", &data.max[0], &data.max[1]);
    // sprintf(msg, "TEXTAT YAW  %.3f,%.3f ", &data.max[0], &data.max[1]);
 
    //  sprintf(msg, "TEXTAT YAW  %.3lf,%.3lf", data.max[0], data.max[1]);
    //     logit(msg);

    // sscanf(msg, "%.3f,%.3f", &data.max[0], &data.max[1]);
  
    createTargets(NUM_TARGETS);


    while(1) {
        readSocket();
        if(strcmp(reced,"GE")){
            //create targets
          createTargets(NUM_TARGETS);

        }
            //  write(targets_server[1], &data, sizeof(data));
        usleep(50000);

    // int client_socket;
    // struct sockaddr_in server_addr;
    // char buffer[255];

    // // Create client socket
    // client_socket = socket(AF_INET, SOCK_STREAM, 0);

    // // Configure server address
    // server_addr.sin_family = AF_INET;
    // server_addr.sin_port = htons(PORT);
    // server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Change to the server's IP address

    // // Connect to the server
    // connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // logit("[Targets]: con");

    }

   while (1) {
//   //      read(server_targets[0], &data, sizeof(data));
//         // Check if drone reached any targets
//         for (int i = targetReached; i < NUM_TARGETS; i++) {
//             if (data.targets[i * 2] != -1) {
//                 targetReached = i;
//                 break;
//             }
//         }
//         // Check the distance between the drone and the target
//         double distance = sqrt(pow(data.drone_pos[0] - data.targets[targetReached * 2], 2) +
//                                 pow(data.drone_pos[1] - data.targets[targetReached * 2 + 1], 2));

//         if (distance < THRESH_TOUCH) {
//             // Drone has reached the target, update the target status
//             data.targets[targetReached * 2] = -1;
//             data.targets[targetReached * 2 + 1] = -1;

//             data.targetReached = targetReached;
//             // Write the updated data to the targets_server pipe
//       //      write(targets_server[1], &data, sizeof(data));
//             // Log the information
//             sprintf(msg, "[Targets]: Drone reached target %d at (%f, %f)", targetReached, data.targets[targetReached * 2], data.targets[targetReached * 2 + 1]);
//             logit(msg);
//         }
    // } 



    }
}