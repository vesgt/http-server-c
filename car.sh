#!/bin/bash
echo "Compiling, then running...                          CAR!!!!!"
gcc -o server server.c routing.c println.c http_responses.c http_request.c
clear
./server
