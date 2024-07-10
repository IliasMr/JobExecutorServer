#!/bin/bash


if [ $# -eq 0 ]; then                                                  #check for arguments 
    echo "No arguments supplied, please run again with arguments"
    exit 1
fi


for file in "$@"; do                                                    #iterate over each txt file 
    
    if [ ! -f "$file" ]; then                                           #check if the txt file given actually exists
        echo "File '$file' not found."
        continue
    fi
    
    while IFS= read -r argument; do                                             #read each line of the txt             
        if [[ -z "$argument" ]]; then                                              #skip for an empty line 
            continue
        fi
        ../bin/./jobCommander linux11.di.uoa.gr 7857 issueJob "$argument"           #assuming that server runs on linux11.di.uoa.gr

        
    done < "$file"
done
