#!/bin/sh

rm results2.txt
touch results2.txt

echo -e "Forking Server Throughput Results" >> results2.txt
echo -e "\n\nTrial 1: Kilobyte\n" >> results2.txt
./thor.py -h 3 -t 10 http://student05.cse.nd.edu:9232/kilofile.txt >> results2.txt
echo -e "\n\nTrial 2: Megabyte\n" >> results2.txt
./thor.py -h 3 -t 10 http://student05.cse.nd.edu:9232/megafile.txt >> results2.txt
echo -e "\n\nTrial 3: Gigabyte\n" >> results2.txt
./thor.py -h 3 -t 10 http://student05.cse.nd.edu:9232/gigafile.txt >> results2.txt

