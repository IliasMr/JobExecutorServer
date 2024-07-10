#define MAX_COMMAND 200

typedef struct job{
    int jobID;                                          //id of a job
    char job[MAX_COMMAND];                              //the command to run
    int clientSocket;                                   //sockets file descriptor 
    int pid;                                            //actual pid of a process
}job;