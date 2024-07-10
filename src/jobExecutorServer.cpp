#include "../includes/headers.h"
#include "../includes/command.h"


using namespace std;

int threadPoolSize = 0;                                                 //global variable for thread pool size 
int portNum;                                                            //port number
int concurrencyLevel = 1;                                               //max concurrency level - default value = 1
int bufferSize;                                                         //size of the buffer
int CurrConc = 0;
int toStop = 0;

int jobID = 0;

pthread_mutex_t concMutex = PTHREAD_MUTEX_INITIALIZER;                   //initialize mutex for concurrencyLevel
pthread_mutex_t jobMutex = PTHREAD_MUTEX_INITIALIZER;                   //initialize mutex for jobID
pthread_mutex_t crrConcMutex =PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t jobCond = PTHREAD_COND_INITIALIZER;                      //initialize conditional variable for workers
pthread_cond_t contrCond = PTHREAD_COND_INITIALIZER;                    //initialize conditional variable for controllers  

vector <job> toRun;                                                     //shared buffer 

pthread_t *workersPtr;
pthread_t main_thread_id;


void handlerCHILD(int signum){
    pthread_mutex_lock(&crrConcMutex);
    CurrConc--;                                                         //a child terminated - reduce current conc 
    pthread_mutex_unlock(&crrConcMutex);
}

void handlerSIGUSR1(int signum) {                                       //SIGUSR1 handler
   
    for (int i = 0; i < threadPoolSize; i++) {
        pthread_join(workersPtr[i], NULL);                              //join each worker thread
    }
    pthread_exit(NULL);                                                 //exit main thread
}



void* worker_thread(void *args){                                                 //worker thread function

    char* argtoken;
    char jobtemp[MAX_COMMAND] = {0};                                            //for saving command                      
    char *execargs[1024];                                                       //for execvp 
    int i = 0;                                                                  //for execargs array
    while(1){                                                                   
        pthread_mutex_lock(&jobMutex);
        while (toRun.empty() || CurrConc >= concurrencyLevel){                  //block if buffer is empty or current conc is bigger than max conc 
            if(toStop == 1){                                                    //server received exit command
                pthread_mutex_unlock(&jobMutex);
                pthread_exit(NULL);
            }
            pthread_cond_wait(&jobCond, &jobMutex);                             
        }

        job j = toRun.front();                                                  //get the first job on the queue                                                             
        toRun.erase(toRun.begin());                                             //remove it from the queue
        pthread_mutex_unlock(&jobMutex);                                        
        strcpy(jobtemp, j.job);
        i=0;                                                                       //reset i

        argtoken = strtok(jobtemp, " ");                                          //create execargs for execvp
        while(argtoken != NULL){
            execargs[i++] = argtoken;
            argtoken = strtok(NULL, " ");
        }
        execargs[i] = NULL;     

        //printf("Worker thread %lu executing job ID %d with command: %s\n", pthread_self(), j.jobID, j.job);                 /// to remove 
        
        pid_t id = fork();                                                      //fork a new child process to run the job
        if(id <0){
            perror("fork");
            exit(EXIT_FAILURE);                                               
        }
        else if (id == 0){

            FILE* file;
            int fdDup;                                                          //file descriptor for the file 
            char filename[20] = {0};                                            //for the file name 
            pid_t childPid = getpid();                                          //get the child pid 
            sprintf(filename, "%d.output", childPid);                           //create the filename 
            file = fopen(filename, "w");                                         //create pid.output  

            fdDup = fileno(file);                                                //get file fd
            if (fdDup == -1){                                                   
                perror("fileno");
                fclose(file);
                exit(EXIT_FAILURE);
            }
            if (dup2(fdDup, STDOUT_FILENO) == -1){                              //redirect stdout to the respective pid.output file 
                perror("dup2");
                fclose(file);
                exit(EXIT_FAILURE);
            }
            fclose(file);

            execvp(execargs[0], execargs);                                      //run the job with execvp
            perror("execvp");                                                   //error if execvp returns 
            printf("There is no such command, please run again with a valid command\n");
            exit(EXIT_FAILURE);

        }
        else{
            pthread_mutex_lock(&crrConcMutex);
            CurrConc++;                                                           //a new child process has been created (a new job is running)
            pthread_mutex_unlock(&crrConcMutex);
            char filename[20] = {0};                                            
            sprintf(filename, "%d.output", id);                                  //create filename 

            int status;                          
            waitpid(id, &status, 0);                                            //wait for the child to finish execution 

            int filefd;
            struct stat fileStat;                                               //using stat to get info for the file we about to read
            if(stat(filename, &fileStat) != 0){                                 //get info for the file (we want the size)
                perror("stat");
                exit(EXIT_FAILURE);
            }
            filefd = open(filename, O_RDONLY);                                  //open pid.output file to read the output 
            if (filefd == -1){
                perror("open on the output file");
                exit(EXIT_FAILURE);
            }
            char* outputStr = (char*)malloc((fileStat.st_size + 1)*sizeof(char));    //to save the output
            
            read(filefd, outputStr, fileStat.st_size);                              //read from the pid.output file 
            outputStr[fileStat.st_size] = '\0'; 
            close(filefd);                                                          //we dont need files fd no more

            int outputSize = fileStat.st_size;
            write(j.clientSocket, &outputSize, sizeof(int) );                       //write the size of the output for the commander to read
            write(j.clientSocket, outputStr, fileStat.st_size );                    //write the job output for the commander to read 

            close(j.clientSocket);                                                  //we dont need the socket no more 

            free(outputStr);

            if (toStop == 1){
                pthread_exit(NULL);
            }

            
        }

    }
  
    pthread_exit(NULL);
}



void* controller_thread(void *args){                                            //controller thread function

    int connfd = *((int *)args);
    char* token;
    char *commandType;                                  //the type of the input command
    char *commandArgs;                                  //arguments for the command 
    char cjob[MAX_COMMAND] = {0};
    char socketBuff[MAX_COMMAND] = {0};

    pthread_mutex_lock(&jobMutex);                                            
    while ((int)toRun.size() >= bufferSize){                                                  //block if buffer is full  
        pthread_cond_wait(&contrCond, &jobMutex);                             
    }
    pthread_mutex_unlock(&jobMutex);                                          

    read(connfd, socketBuff, sizeof(socketBuff) - 1);                                   //read the command from the socket 
    token = strtok(socketBuff, " ");
    if (token != NULL){                                                             //split the input into 2 strings: commandType and commandArgs
        commandType = token;
        token = strtok(NULL, "");
        commandArgs = token;
        if(token != NULL)                                                       //if there is a seconds argument                                   
            strcpy(cjob, token);                                                 //copy it to cjob (if commandType != issueJob we wont use the cjob)
    }
    
    if(strcmp(commandType, "issueJob") == 0){                               
        char response[MAX_COMMAND+20] = {0};
        pthread_mutex_lock(&jobMutex);                                            
        jobID++;                                                                  //a new job for the buffer
    //    pthread_mutex_unlock(&jobMutex);                                           
      //  pthread_mutex_lock(&jobMutex);
        job j;
        j.jobID = jobID;                                                            //save jobID
        j.clientSocket = connfd;                                                    //save socket file descriptor 
        strcpy(j.job, cjob);
        toRun.push_back(j);                                                         //push the job to the buffer 
        sprintf(response, "JOB %d, %s SUBMITTED", j.jobID, j.job);
        write(connfd, response, strlen(response));                                  //send response back to the commander

        pthread_cond_broadcast(&jobCond);                                           //wake up any available worker 

        printf("-----\n");
        pthread_mutex_unlock(&jobMutex);                                            //release the mutex 
    }
    else if(strcmp(commandType, "setConcurrency") == 0){
        int newconc = atoi(commandArgs);                                       //convert to int  
        char response[30] = {0};

        if(newconc - concurrencyLevel == 1){                                        //wake up only one worker if conc changed by 1 
            pthread_cond_signal(&jobCond);                                  
        }

        else if(newconc > concurrencyLevel)                                          //if we are able to run a new job
            pthread_cond_broadcast(&jobCond);                                       //wake up any blocked worker to run a job (if any)

        pthread_mutex_lock(&concMutex);                     
        concurrencyLevel = newconc;                                             //change max concurrency level
        pthread_mutex_unlock(&concMutex);
        sprintf(response,"CONCURRENCY SET AT %d\n", concurrencyLevel);                   //create response for Commander
         
        write(connfd, response, strlen(response));                                       
        close(connfd);                                                             //close socket fd as we dont need it any more 

    }else if(strcmp(commandType, "poll") == 0){
     
        int toRunSize;                                                              //size of toRun buffer 
        pthread_mutex_lock(&jobMutex);
        toRunSize = toRun.size();   
        write(connfd, &toRunSize, sizeof(int));                                               
        if(toRunSize > 0){
            job toSend;
            for (int l = 0; l<(int)toRun.size(); l++){                   //send every job in the buffer back to Commander for printing 
                toSend = toRun[l];
                write(connfd, &toSend, sizeof(job));                                               
            }
        }
        pthread_mutex_unlock(&jobMutex);
        close(connfd);                                                              //close connfd as we dont need it any more 

        
    }else if(strcmp(commandType, "stop") == 0){
        int toStopId;
        char* toStopStr;
        char* jobToken;
        int found = 0;                                                      //to check if the id found 
        char response[50] = {0};

        jobToken = strtok(commandArgs, "_");                                //split commandargument at _
        if(jobToken != NULL){
            toStopStr = strtok(NULL, "\0");                                 //get id as string
        }
        toStopId = atoi(toStopStr);                                         //convert it to int 

        pthread_mutex_lock(&jobMutex);
        if (toRun.empty() != true){                                         //search the buffer for the id 
            for(int k=0; k<(int)toRun.size(); k++){
                if(toRun[k].jobID == toStopId){
                    found = 1;
                    sprintf(response, "JOB_%d REMOVED\n", toStopId);            //create response for the commander
                    close(toRun[k].clientSocket);
                    toRun.erase(toRun.begin() + k);                             //remove jobID from the buffer 
                    break;
                }
            }
        }
        pthread_mutex_unlock(&jobMutex);

        if (found == 0){                                                        //jobID not found in the buffer
            sprintf(response, "JOB_%d NOT FOUND\n", toStopId);
        }

        write(connfd, &response, sizeof(response));                            //send response back to the commander       
        close(connfd);                                                          //close connfd as we dont need it any more

    }else if (strcmp(commandType, "exit") == 0){
        
        char response[50] = "SERVER TERMINATED BEFORE EXECUTION\n";             //response for each client who waits for execution
        char responseToClient[40] = "SERVER TERMINATED\n";                      //response for the client who sent the exit command
        
        write(connfd, &responseToClient, sizeof(responseToClient));             //write response to the client who sent the exit command
        close(connfd);                                                          //we dont need the clients fd no more 
        
        pthread_mutex_lock(&jobMutex);
        int size = sizeof(response);    
        //check for empty buffer ???
        for(int k=0; k<(int)toRun.size(); k++){                                 //for each job in the buffer
            write(toRun[k].clientSocket, &size, sizeof(int));                   
            write(toRun[k].clientSocket, &response, sizeof(response));          //write the response to the respective client 
            close(toRun[k].clientSocket);                                       //close each client fd as we dont need it no more 
        }
        toRun.clear();                                                          //remove all jobs from the buffer
        pthread_mutex_unlock(&jobMutex);    

        toStop = 1;                                                             //mark toStop flag

        pthread_mutex_lock(&jobMutex);
        pthread_cond_broadcast(&jobCond);                                       //wake up any blocked workers to terminate 
        pthread_mutex_unlock(&jobMutex);
      
        pthread_kill(main_thread_id, SIGUSR1);                                  //signal the mainthread to exit
        free(args);
        pthread_exit(NULL);
    }   

    free(args); 
    pthread_exit(NULL);                                                         //exit controller thread 

}


void* main_thread(void *args){                                          //main thread function                             
    
    pthread_t workers[threadPoolSize];                                  //array to hold workers 
    workersPtr = workers;                                               //pointer to workers array 
    int listenfd;                                                       //file descriptor for the socket 
    struct sockaddr_in servaddr;                                        //struct for server address
    //socklen_t addr_len;                                                         

    struct sigaction sa_usr1;
    sa_usr1.sa_handler = handlerSIGUSR1;
    sa_usr1.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    for (int i = 0; i < threadPoolSize; i++) {                                              //create threadpool with worker threads
        if (pthread_create(&workers[i], NULL, worker_thread, NULL) != 0){
            perror("Workers thread");
            exit(EXIT_FAILURE);
        }
    }
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){                                 //create the socket for communication with the commander
        perror("socket");
        exit(EXIT_FAILURE);
    }
    bzero(&servaddr, sizeof(servaddr));                                                     //initialize server address struct 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(portNum); 
    if ((bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr))) < 0){                     
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if((listen(listenfd, 128)) < 0){                                                             //listen for incoming connections, backlog -> 128
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while(1){                                                                                   //continually accept new connections 
        if(toStop == 1){
            break;
        }    
        int *connfd = (int*)malloc(sizeof(int));                                               //file descriptor for the accept 
        struct sockaddr_in clientaddr;                                                          //struct for client adress
        socklen_t clientlen = sizeof(clientaddr);
        if( (*connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen)) < 0){       //accept a new connection
            perror("accept");
            exit(EXIT_FAILURE);
        }
        pthread_t contr_id;
        if (pthread_create(&contr_id, NULL, controller_thread, connfd) != 0) {                  //create a controller thread to handle the connection 
            perror("Controller thread");
            continue;
        }
        pthread_detach(contr_id);                                                               //controller thread will run independently 
    }

    close(listenfd);

    // for (int i = 0; i < threadPoolSize; i++) {                                               
    //     pthread_join(workers[i], NULL);                                                         //main waits for all workers to finish 
    // }

    pthread_exit(NULL);                                                                 

}




int main(int argc, char** argv){


    if(argc < 4){
        printf("Wrong input, please run jobCommander again with a valid arguments..\n");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa_child;                                                       //SIGCHLD install
    sa_child.sa_handler = handlerCHILD;
    sa_child.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa_child, NULL);

    portNum = atoi(argv[1]);                                                            //save command line args 
    if(portNum == 0){                                                                   //wrong portNum input
        printf("Please run again with valid portNum\n");
        exit(EXIT_FAILURE);                                                             
    }

    bufferSize = atoi(argv[2]);                                                  
    if(bufferSize == 0){                                                                //wrong bufferSize input
        printf("Please run again with valid bufferSize\n");
        exit(EXIT_FAILURE);
    }       
    threadPoolSize = atoi(argv[3]);                                                     
    if(threadPoolSize == 0){                                                            //wrong theadPoolSize value 
        printf("Please run again with valid threadPoolSize\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&main_thread_id, NULL, &main_thread, NULL) != 0){                //create the main thread 
        perror("Main thread");
        exit(EXIT_FAILURE);
    }
    pthread_join(main_thread_id, NULL);                                                 //wait for the main thread to finish

    pthread_mutex_destroy(&jobMutex);                                                   //destroy mutexes
    pthread_mutex_destroy(&crrConcMutex);
    pthread_mutex_destroy(&concMutex);
    
    pthread_cond_destroy(&jobCond);                                                     //destroy cond variables 
    pthread_cond_destroy(&contrCond);

    return 0;

}