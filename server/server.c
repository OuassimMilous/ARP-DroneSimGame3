#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/select.h>
#include <unistd.h> 
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include "include/constants.h"
#include <arpa/inet.h>
#include <stdbool.h>


// declaration of variables
sem_t *LOGsem;
FILE *file;
time_t start_time;  
int centered =0;
int port;
char msg [SOCEKTMSGLEN];
char reced[SOCEKTMSGLEN];
char buffer[SOCEKTMSGLEN];
char bufferrec[SOCEKTMSGLEN];
int targetnum = NUM_TARGETS;
double obst_pos[NUM_OBSTACLES*2];
double targets_pos[NUM_TARGETS*2];
struct data data, updated_data;
int server_UI[2];
int server_keyboard[2];
int server_drone[2];
int rec_pipes[NUM_PROCESSES-2][2];
int server_socket, client_socket,client_socket2,client_target;
struct sockaddr_in server_addr, client_addr,client_addr2;
socklen_t addr_size = sizeof(struct sockaddr_in);

// the function to log things
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


// a function to write on the socket and wait for an echo
void writeSocket(char *msgs,int client_socket){
    do{

        //write the message
        bzero(buffer, SOCEKTMSGLEN);
        strncpy(buffer, msgs, SOCEKTMSGLEN - 1);  
        
        ssize_t bytes_written = write(client_socket, buffer, strlen(buffer));
        if (bytes_written <= 0) {
            perror("matrytawch");
            exit(EXIT_FAILURE);
        }

        sprintf(msg,"[Server]: socket sent : %s", buffer);
        logit(msg);

        //read the echo
        bzero(bufferrec, SOCEKTMSGLEN);
        if (read(client_socket, bufferrec, sizeof(bufferrec) - 1) == -1) {
            perror("writeSocket");
            exit(EXIT_FAILURE);
        }

        sprintf(msg,"[Server]: socket echo: %s", bufferrec);
        logit(msg);


        usleep(SOCKETWAIT);
    //wait for a while and check if everything went well, or else, you try again
    } while (strncmp(buffer, bufferrec, SOCEKTMSGLEN - 1) != 0);
}


// a function to read on the socket and wait for an echo
void readSocket(int client_socket) {
 do
 {
   // Read the message from the client
    bzero(buffer, SOCEKTMSGLEN);
    if (read(client_socket, buffer, sizeof(buffer) - 1) == -1) {
        perror("readSocket");
        exit(EXIT_FAILURE);
    }

    strncpy(reced, buffer, SOCEKTMSGLEN - 1);  

    sprintf(msg,"[Server]: socket recieved: %s", buffer);
    logit(msg);

    // Echo the message back to the client
    bzero(bufferrec, SOCEKTMSGLEN);
    strncpy(bufferrec, buffer, SOCEKTMSGLEN - 1);  
    ssize_t bytes_written = write(client_socket, bufferrec, strlen(bufferrec));
    if (bytes_written <= 0) {
        perror("matrytawch< ta3");
        exit(EXIT_FAILURE);
    }

    sprintf(msg,"[Server]:Sent echo back client: %s", bufferrec);
    logit(msg);

    //wait for a while and check if everything went well, or else, you try again
    usleep(SOCKETWAIT);
    } while (strncmp(buffer, bufferrec, SOCEKTMSGLEN - 1) != 0);

}

// a function to parse the data recieved from sockets
void handle_socket_reced(int client){
    readSocket(client);

    if (strcmp(reced, "TI") == 0) {
        // Code for "TI"
        sprintf(msg, "%.3lf,%.3lf", data.max[0], data.max[1]);
        writeSocket(msg,client);
        client_target = client;
    }else if (reced[0] == 'T'&&reced[1] == '[') {
        // Initialize targetReached to 0
        data.targetReached = 0;

        char currentNumber[NUM_TARGETS*2];  
        int numbersCount = 0;               
        int currentNumberIndex = 0;      
        int first = 0;
        // Iterate through each character in the received string until the null terminator is encountered
        for (int i = 0; reced[i] != '\0'; i++) {
            char c = reced[i];

            // Check if the character is a digit or a decimal point
            if (isdigit(c) || c == '.') {
                currentNumber[currentNumberIndex++] = c;
            } else if (currentNumberIndex > 0) {
                // If a number has been collected
                // Null-terminate the currentNumber array
                currentNumber[currentNumberIndex] = '\0';
                if (numbersCount == 0 && first==0)
                {
                    data.targetnum =  (double)atof(currentNumber);
                    first = 1;
                }else{
                    // Convert the string to a double and store it
                    data.targets[numbersCount++] = (double)atof(currentNumber);
                }
                // Reset the index for the next number
                currentNumberIndex = 0;
            }
        }

        // Check if there is a number at the end of the input
        if (currentNumberIndex > 0) {
            currentNumber[currentNumberIndex] = '\0';  // Null-terminate the current number string
            data.targets[numbersCount++] = atof(currentNumber);
        }     
        // Output the numbers
        for (int i = 0; i < numbersCount; i++) {
            // Print each value using the logit function
            sprintf(msg, "%f ", data.targets[i]);
            logit(msg);
        }

        // Send the data structure through a socket
        write(server_UI[1], &data, sizeof(data));


    // Handling "OI" Command
    } else if (strcmp(reced, "OI") == 0) {
        // If the received string is "OI", construct a message with maximum values and send it back
        sprintf(msg, "%.3lf,%.3lf", data.max[0], data.max[1]);
        writeSocket(msg, client);


    // Parsing Obstacle Data
    } else if (reced[0] == 'O' && reced[1] == '[') {
        char currentNumber[NUM_TARGETS*2];  
        int numbersCount = 0;               
        int currentNumberIndex = 0;      
        int first = 0;
        // Iterate through each character in the received string until the null terminator is encountered
        for (int i = 0; reced[i] != '\0'; i++) {
            char c = reced[i];

            // Check if the character is a digit or a decimal point
            if (isdigit(c) || c == '.') {
                currentNumber[currentNumberIndex++] = c;
            } else if (currentNumberIndex > 0) {
                // If a number has been collected
                // Null-terminate the currentNumber array
                currentNumber[currentNumberIndex] = '\0';
                
    
                if (numbersCount == 0 && first==0)
                {
                    data.obsnum =  (double)atof(currentNumber);
                    first = 1;
                }else{
                    // Convert the string to a double and store it in data.obstacles
                    data.obstacles[numbersCount++] = (double)atof(currentNumber);
                }
                // Reset the index for the next number
                currentNumberIndex = 0;
            }
        }

        // Check if there is a number at the end of the input
        if (currentNumberIndex > 0) {
            currentNumber[currentNumberIndex] = '\0';  // Null-terminate the current number string
            data.obstacles[numbersCount++] = atof(currentNumber);
        }

        // Output the numbers
        for (int i = 0; i < numbersCount; i++) {
            sprintf(msg,"%f ",  data.obstacles[i]);
            logit(msg);
        }

        logit( "[Server]: Obstacles received ");
        write(server_UI[1],&data,sizeof(data));
    }
}

int main(int argc, char *argv[]) {
  
    //get the port from the user (transfered by the Master)
    port = atoi(argv[0]);

    // open the semaphore for logging
    LOGsem = sem_open(LOGSEMPATH, O_RDWR, 0666); // Initial value is 1
    if (LOGsem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    sem_post(LOGsem);

   //Rec_pipes in order of
    /*
    WINDOW
    KEYBOARD
    DRONE
    */
    sscanf(argv[1],"%d %d|%d %d|%d %d|%d %d|%d %d|%d %d",
                                    &rec_pipes[0][0],   &rec_pipes[0][1], &server_UI[0],       &server_UI[1],
                                    &rec_pipes[1][0],   &rec_pipes[1][1], &server_keyboard[0], &server_keyboard[1],
                                    &rec_pipes[2][0],   &rec_pipes[2][1], &server_drone[0],    &server_drone[1]); // Get the fds of the pipe to watchdog
    
    // Close unnecessary pipes
    close(server_drone[0]); 
    close(server_keyboard[0]);
    close(server_UI[0]);

    for(int i=0; i< NUM_PROCESSES-1; i++){
        close(rec_pipes[i][1]);
    }
  
    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind the socket to the address and port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
     // Listen for incoming connections
    listen(server_socket, 3);

    // Accept two clients
    printf("Server listening on port %d...\n", port);

    //uncomment to make this non blocking

    // int flags = fcntl(server_socket, F_GETFL, 0);
    // fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);

    // Accept connection from client 2
    client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
    if (client_socket < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    // Accept connection from client 2
    client_socket2 = accept(server_socket, (struct sockaddr*)&client_addr2, &addr_size);
    if (client_socket2 < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    while(data.exit_flag != 1){

        //set up the select
        fd_set reading;
        FD_ZERO(&reading);
        
        for (int i = 0; i < (NUM_PROCESSES-2); i++) {
            FD_SET(rec_pipes[i][0], &reading);
        }
            FD_SET(client_socket, &reading);
            FD_SET(client_socket2, &reading);

        // set max_pipe_fd
        int max_pipe_fd = -1;  
        for (int i = 0; i < (NUM_PROCESSES-2); i++) {
            if (rec_pipes[i][0] > max_pipe_fd) {
                max_pipe_fd = rec_pipes[i][0];
            }
        }
        if (client_socket > max_pipe_fd) {
            max_pipe_fd = client_socket;
        }
        if (client_socket2 > max_pipe_fd) {
            max_pipe_fd = client_socket;
        }

    
        // selecting which pipe/socket is recieving data
        int ret_val = select(max_pipe_fd + 1, &reading, NULL, NULL, NULL);

        if (ret_val == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        if(ret_val>0){
            // pipes
            for(int j=0; j<(NUM_PROCESSES-2); j++){
                if(FD_ISSET(rec_pipes[j][0],&reading)>0){ // Only from pipes that are updated
                    read(rec_pipes[j][0],&updated_data, sizeof(data)); 
                    switch (j){
                        case 0: //UI
                            memcpy(data.max, updated_data.max, sizeof(updated_data.max));
                            if (!centered){
                                memcpy(data.drone_pos, updated_data.drone_pos, sizeof(updated_data.drone_pos));
                                data.Cobs_touching=updated_data.Cobs_touching; // Update shared data with the updated variables
                                logit( "[Server]:centered pos recived and sent to drone and target");
                                centered = 1;
                            }
                            // data.game_over = updated_data.game_over;
                            
                            logit( "[Server]: max updated");
                            write(server_drone[1],&data,sizeof(data));
                        break;
                        
                        case 1: //keyboard
                            if (centered)
                            {
                                data.key=updated_data.key; // Update shared data with the updated variables
                                write(server_drone[1],&data,sizeof(data));
                                sprintf(msg, "[Server]: key %c recieved and sent to Drone", data.key);
                                logit(msg);
                            }
                        break;

                        case 2: //drone
                            // double position[6];
                            memcpy(data.drone_pos, updated_data.drone_pos, sizeof(updated_data.drone_pos));
                            sprintf(msg, "[Server]: New pos drone sent to UI %f,%f", data.drone_pos[0],data.drone_pos[1]);
                            logit(msg);
                            data.Cobs_touching=updated_data.Cobs_touching; 
                            data.exit_flag=updated_data.exit_flag; 
                            memcpy(data.targets, updated_data.targets, sizeof(updated_data.targets));
                            write(server_UI[1],&data,sizeof(data));
                        break;

                    }
                }
            }

            // sockets

            // first client
            if (FD_ISSET(client_socket, &reading)>0){
                handle_socket_reced(client_socket);
            }
            
            if (FD_ISSET(client_socket2, &reading)>0){
                handle_socket_reced(client_socket2);
            }
        }  

        // checking if the user have won
        bool all_targets_hit = true;
        for (int i = 0; i < data.targetnum  * 2; ++i) {
            if (data.targets[i] != -1) {
                all_targets_hit = false;
                break;
            }
        }

        // restart the game if the user has won
        if (all_targets_hit && data.targetnum>0) {
            //wait for a bit
            sleep(4);
            //request new targets
            writeSocket("GE",client_target);
            // recieve the targets
            handle_socket_reced(client_target);

            //put the drone back to its original position
            data.key = 'x';
            data.drone_pos[0] = data.max[0] / 2;    
            data.drone_pos[1] = data.max[1] / 2;  
            data.drone_pos[2] = data.max[0] / 2;  
            data.drone_pos[3] = data.max[1] / 2;  
            data.drone_pos[4] = data.max[0] / 2;  
            data.drone_pos[5] = data.max[1] / 2;  
            write(server_drone[1],&data,sizeof(data));
        }

    }

    // send a request to close the program from the client side as well
    writeSocket("stop",client_socket2);
    writeSocket("stop",client_socket);

    // clean up
    close(server_drone[1]);
    close(server_keyboard[1]);
    close(server_UI[1]);
    close(client_socket);
    for(int i=0; i< NUM_PROCESSES-1; i++){
        close(rec_pipes[i][0]);
    }


    return 0;
}
