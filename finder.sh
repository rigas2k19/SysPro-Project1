#!/bin/bash

# Get number of parameters passed
echo "Number of arguments is : $#"
counter=$#
if [[ $counter -eq $zero ]]; then
    echo "No arguments were passed"
    exit 3
fi

# for every TLD passed as argument
for (( var=1; var<=counter; var++ ))
do
    # get the var-th argument
    tld=${!var}
    # get length of tld
    len=${#tld}
         
    numberOfAppearences=0
    
    # get all the .out files in output folder
    for file in output/*.out
    do
        # Read each file line by line
        while read -r line
        do  
            # Split line by token " "
            iterations=0
            IFS=' '
            read -ra ADDR <<< "$line"
            for i in "${ADDR[@]}"; do
                # First token is urls location
                if [[ $iterations -eq $zero ]]; then
                    location=$i
                fi
                # Second token is urls appearences in file
                if [[ $iterations == 1 ]]; then
                    # Check if location ends in TLD
                    if [[ "$location" == *"$tld" ]]; then
                        number=$i
                        ((numberOfAppearences=numberOfAppearences+number)) 
                    fi
                fi
                ((iterations=iterations+1))
            done
        done < $file
    done
    echo "TLD : $tld number of appearences is $numberOfAppearences"
done