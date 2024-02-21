#ifndef CONSTANTS_H
#define CONSTANTS_H


#define LOGPATH "./log/logfile.txt"
#define LOGSEMPATH "/LOGsemcli"

#define NUM_PROCESSES 3
#define NUM_OBSTACLES 5
#define NUM_TARGETS 5
#define SOCEKTMSGLEN 1024
#define SOCKETWAIT 100

struct data{
    double obstacles[NUM_OBSTACLES*2];
    double targets[NUM_TARGETS*2];
    double max[2];
};



#endif // !CONSTANTS_H
