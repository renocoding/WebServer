#!/bin/bash

THOR="./bin/thor_test.py"
FLAGS="-h 1 -t 4"
ADDR="http://student06.cse.nd.edu:9898"
TARGET_1KB="/test_files/1KB.txt"
TARGET_1MB="/test_files/1MB.txt"
TARGET_1GB="/test_files/1GB.txt"


#$THOR $FLAGS "$ADDR$TARGET_1KB"


echo "1KB Times"
for i in $(seq 5); do
 $THOR $FLAGS "$ADDR$TARGET_1KB" | grep "AVERAGE ELAPSED TIME" | grep -Eo ".{1}\..*"

done

echo "1MB Times"
for i in $(seq 5); do
 $THOR $FLAGS "$ADDR$TARGET_1MB" | grep "AVERAGE ELAPSED TIME" | grep -Eo ".{1}\..*"
done

echo "1GB Times"
for i in $(seq 5); do
 $THOR $FLAGS "$ADDR$TARGET_1GB" | grep "AVERAGE ELAPSED TIME" | grep -Eo ".{1}\..*" 
done


