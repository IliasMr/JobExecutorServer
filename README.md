Web socket application written in C/C++ that controls parallel execution of tasks using a thread pool.  
Each jobCommander client connects to the server via sockets using TCP connection.  
We can interact with the server by sending commands - jobs for controlled execution.



**Compilation**:
- **$make**

**jobExecutorServer**: jobExecutorServer [portNum] [bufferSize] [threadPoolSize]
    
- e.g. **$./jobExecutorServer 7856 10 10**
   

**jobCommander** : jobCommander [serverName] [portNum] [jobCommanderInputCommand]

- **issuejob**:  insert a new job on the system   
    e.g. **$./jobCommander localhost 7856 issueJob ls -l**
    
- **setConcurrency N**: sets the max number of jobs that can be run in parallel  
    e.g. **$./jobCommander localhost 7856 setConcurrency 4**
    
- **stop jobID**: removes job from the buffer   
    e.g. **$./jobCommander localhost 7586 stop job_1**

- **poll**: prints the jobs that wait for execution  
    e.g. **$./jobCommander localhost 7586 poll**
    
- **exit**: terminates jobExecutorServer  
    e.g. **$./jobCommander localhost 7856 exit**


**Multijob.sh script** :
    
- takes as input commands (txt files) and assigns them to the server for execution   
    e.g. **$./multijob.sh commands_1.txt commands_2.txt**

        
This application was a university project in my SystemProgramming course.
    	

