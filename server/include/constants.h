#ifndef CONSTANTS_H
#define CONSTANTS_H


#define LOGPATH "./log/logfile.txt"
#define LOGSEMPATH "/LOGsem"


#define M 1.0
#define K 1.0
#define T 1
#define N 2
#define FORCEX 1.0
#define FORCEY -0.25



#define SOCEKTMSGLEN 1024
#define NUM_PROCESSES 5
#define NUM_OBSTACLES 20
#define NUM_TARGETS 20
#define THRESH_TARGET 10
#define THRESH_TOUCH 3
#define THRESH_TOUCHOBS 1
#define SOCKETWAIT 100


struct data{
    double drone_pos[6]; // Array to store the position of drone
    double obstacles[NUM_OBSTACLES*2];
    double targets[NUM_TARGETS*2];
    char key;
    double max[2];
    int Cobs_touching;
    int targetReached;
    int obsnum;
    int targetnum;
    int exit_flag;
};



#endif // !CONSTANTS_H
