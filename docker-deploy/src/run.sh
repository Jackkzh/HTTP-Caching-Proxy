#!/bin/bash
make clean
make all
echo 'start running proxy server...'
./main &
while true ; do continue ; done