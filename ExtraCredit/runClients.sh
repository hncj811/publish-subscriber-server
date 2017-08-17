#!/bin/bash

display_usage()
{
    echo -n "Usage: ./runClients.sh #_clients"
    echo ""
    exit
}

[[($# -lt 1)]] && display_usage

output_result="output${1}.dat"

#Clean output
[[ -f $output_result ]] && rm $output_result

cmd1="client imp4.cs.clemson.edu 5000 1 1 3"
cmd2="client imp4.cs.clemson.edu 5000 1 2"

test1="client imp4.cs.clemson.edu 5000 0 1 0.5 50 10"
test2="client imp4.cs.clemson.edu 5000 0 2 0.5 50 10"
test3="client imp4.cs.clemson.edu 5000 0 3 0.5 50 10"


for(( i=1; i <= $1 ; i++))
do
    if [ $((i % 2)) -eq 0 ];
    then
	./$cmd1 >> $output_result 2>&1 &
    else
	./$cmd2 >> $output_result 2>&1 &
    fi

    echo "Running Client: $i"
done

echo "**************************"
echo ""
echo "Sending Topic 1"
./$test1
echo "Sending Topic 2"
./$test2
echo "Sending Topic 3"
./$test3