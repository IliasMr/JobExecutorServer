#include "../includes/headers.h"
#include "../includes/command.h"



int main(int argc, char** argv){

    int i;
    char serverName[50] = {0};                                                      
    char resolvedip[50] = {0};                                                      //to hold resolved ip
    int portNum;
    char commandstr[MAX_COMMAND] = {0};                                             //jobCommanderInputCommand
    int  clientfd;      
    struct sockaddr_in servaddr;

    if(argc < 4){
        printf("Wrong input, please run jobCommander again with a valid arguments..\n");
        exit(EXIT_FAILURE);
    }

    if(strcmp(argv[3], "exit") != 0 && strcmp(argv[3], "issueJob") != 0 && strcmp(argv[3], "setConcurrency") != 0 && strcmp(argv[3], "stop") != 0 && strcmp(argv[3], "poll") !=0)
    {
        printf("Wrong command, please run again with a valid command\n");
        exit(EXIT_FAILURE);
    }

    strcpy(serverName, argv[1]);                                                    //get serverName    
    portNum = atoi(argv[2]);                                                        //get the port number
    if (portNum == 0){                                                              //wrong portNum input 
        printf("Please run again with valid portNum\n");
        exit(EXIT_FAILURE);
    }    
    
    struct hostent *mymachine;                                                      //resolve serverName to an actual IP 
    struct in_addr **addr_list;
    if ((mymachine = gethostbyname(serverName)) == NULL)
        perror("Error on gethostbyname");
        
    else {
        addr_list = (struct in_addr **) mymachine->h_addr_list;
        for (int i=0;addr_list[i] != NULL; i++){
            strcpy (resolvedip , inet_ntoa (* addr_list [ i ]) );
        } 
    }
      
    if(strcmp(argv[3], "exit") == 0){                                                   //parse the input - for exit just cpy the exit command
        strcpy(commandstr, argv[3]);
    }
    else{
        for(i=3;i<argc;i++){                                                                    //if not exit, copy args to commandstr string, splitted by space
            strcat(commandstr,argv[i]);
            if (i < argc -1 )
                strcat(commandstr, " ");
        }
    }
    
    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){                              //open socket for communication with the server 
        perror("Client socket");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, '0', sizeof(servaddr));                                           //intialize servaddr struct
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(portNum);

    if(inet_pton(AF_INET, resolvedip, &servaddr.sin_addr) <= 0){                            //convert ip to binary
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if ( (connect(clientfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) < 0 ){          //initiate connection to the socket
        perror("Connect");
        exit(EXIT_FAILURE);
    }

    write(clientfd, commandstr, strlen(commandstr));                                        //write command to the socket for the server to read it 

    if(strcmp(argv[3], "issueJob") == 0){                                                   //check command type 
        char responseFromServ[MAX_COMMAND+20] = {0};
        read(clientfd, responseFromServ, sizeof(responseFromServ));                         //read the first response from the server
        printf("%s\n", responseFromServ);         

        int outputSize;
        read(clientfd,&outputSize, sizeof(int));                                            //read the size of the output from server 

        char* outputStr = (char*)malloc(sizeof(char)*outputSize);                   
        read(clientfd, outputStr, outputSize);                                              //read the job output from the server 
        if(outputSize != 0)                                                                 //outputSize == 0 -> command has no output 
            printf("%s\n", outputStr);                                          

        free(outputStr);                                                        
    }
    else if(strcmp(argv[3], "setConcurrency") == 0){                               

        char responseFromServ[30] = {0};
        read(clientfd, responseFromServ, sizeof(responseFromServ));                        //read response from the server
        printf("%s\n", responseFromServ);  

    }
    else if(strcmp(argv[3], "poll") == 0){

        int toRunSize;
        read(clientfd, &toRunSize, sizeof(int));                                            //read toRun buffer size from the server
        if(toRunSize == 0){                                                                 //buffer is empty 
            printf("No jobs in the buffer!\n"); 
        }
        else if(toRunSize > 0){
            job toPrint;
            for(int l = 0; l<toRunSize; l++){                                               //for every job in the buffer
                read(clientfd, &toPrint, sizeof(job));
                printf("JobID: %d, job: %s\n", toPrint.jobID, toPrint.job);                 //print job info 
            }
        }

    }
    else if(strcmp(argv[3], "stop") == 0){

        char responseFromServ[50] = {0};
        read(clientfd, &responseFromServ, sizeof(responseFromServ));                        //read response from the server
        printf("%s\n", responseFromServ);
   
    }
    else if (strcmp(argv[3], "exit") == 0){
        
        char responseFromServ[40] = {0};
        read(clientfd, &responseFromServ, sizeof(responseFromServ));                        //read response from the server server
        printf("%s\n", responseFromServ);

    }

    close(clientfd);                                                                 //we are done with clientfd
    return 0;

}