Task handler application written in C/C++ that controls parallel execution of tasks using a thread pool.  
Server must be up - each jobCommander client connects to the server via sockets using TCP connection.  
Server runs in any machine, we can connect to the server by different machines.  



**Compilation**:
- $make

**Server**: jobExecutorServer [portNum] [bufferSize] [threadPoolSize]
    
- e.g. $./jobExecutorServer 7856 10 10
   

**jobCommander** : jobCommander [serverName] [portNum] [jobCommanderInputCommand]

- issuejob:  insert a new job on the system 
    e.g. $./jobCommander localhost 7856 issueJob ls -l
    
- setConcurrency N: sets the max number of jobs that can be run in parallel
    e.g. $./jobCommander localhost 7856 setConcurrency 4
    
- stop <jobID>: removes job from the buffer 
    e.g. $./jobCommander localhost 7586 stop job_1

- poll: prints the jobs that wait for execution
    e.g. $./jobCommander localhost 7586 poll
    
- exit: terminates jobExecutorServer
    e.g. $./jobCommander localhost 7856 exit


**Multijob.sh script** :
    
- takes as input commands (txt files) and assigns them to the server for execution 
    e.g. $./multijob.sh commands_1.txt commands_2.txt

        
This application was a university project in my SystemProgramming course.
    	

