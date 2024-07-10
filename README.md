Task handler application written in C/C++ 


Compilation:
    Using the Makefile:
    	$make
    	
    Execution on different terminals:
  
        $cd bin
        $./jobExecutorServer 7857 10 10

   
        $cd bin
        $./jobCommander localhost 7857 issueJob ls -l          
        $..
        $..
        $./jobCommander localhost 7857 issueJob sleep 100 
        $.
        $.
        $./jobCommander localhost 7857 exit
