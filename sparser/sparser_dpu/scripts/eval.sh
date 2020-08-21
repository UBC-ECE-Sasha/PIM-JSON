#!/bin/bash

# $1 filename $2 number of the queries $3 number of tasklets $4 number of DPUs $5 output file
sed -i '4d' Makefile
sed -i "4 i NR_TASKLETS\ ?=\ $3" Makefile
sed -i '5d' Makefile
sed -i "5 i NR_DPUS\ ?=\ $4" Makefile
# sed -i '6d' Makefile
# sed -i "6 i QUERY\ ?=\ $5" Makefile
make clean
make 
# ./bench $1 5 >> $5
# sleep 10
# ./bench $1 6 >> $5
# sleep 10
# ./bench $1 8 >> $5
# sleep 10
./bench $1 9 >> $5
sleep 10
./bench $1 10 >> $5
sleep 10
./bench $1 11 >> $5
