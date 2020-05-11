#!/bin/bash

THOR="./bin/thor_test.py"
FLAGS="-h 4 -t 3"
ADDR="http://student10.cse.nd.edu:9533"
TARGET_FILE="/text/hackers.txt"
TARGET_DIR="/images"
TARGET_CGI="/scripts/cowsay.sh"
AVERAGE_FILE=0
AVERAGE_DIR=0
AVERAGE_CGI=0



echo "File Times"
for i in $(seq 10); do
 $THOR $FLAGS "$ADDR$TARGET_FILE" | grep "AVERAGE ELAPSED TIME" | grep -Eo ".{1}\..*"

done

echo "Directory Times"
for i in $(seq 10); do
 $THOR $FLAGS "$ADDR$TARGET_DIR" | grep "AVERAGE ELAPSED TIME" | grep -Eo ".{1}\..*"
done

echo "CGI Times"
for i in $(seq 10); do
 $THOR $FLAGS "$ADDR$TARGET_DIR" | grep "AVERAGE ELAPSED TIME" | grep -Eo ".{1}\..*" 
done

