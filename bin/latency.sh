#!/bin/sh

rm results2.txt
touch results2.txt

echo -e "Single Server Latency Results" >> results2.txt
echo -e "\n\nTrial 1: Directory (Root)\n" >> results2.txt
./thor.py -h 4 -t 10 http://student05.cse.nd.edu:9232 >> results2.txt
echo -e "\n\nTrial 2: File (index.html)\n" >> results2.txt
./thor.py -h 4 -t 10 http://student05.cse.nd.edu:9232/html/index.html >> results2.txt
echo -e "\n\nTrial 3: CGI Scripts (cowsay.sh)\n" >> results2.txt
./thor.py -h 4 -t 10 http://student05.cse.nd.edu:9232/scripts/cowsay.sh >> results2.txt

