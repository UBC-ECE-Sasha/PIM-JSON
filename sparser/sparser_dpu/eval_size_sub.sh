#!/bin/bash

# $1 filename $2 number of the queries $3 number of tasklets $4 number of DPUs $5 query name
sed -i '4d' Makefile
sed -i "4 i NR_TASKLETS\ ?=\ $3" Makefile
sed -i '5d' Makefile
sed -i "5 i NR_DPUS\ ?=\ $4" Makefile
sed -i '6d' Makefile
sed -i "6 i QUERY\ ?=\ $5" Makefile
make clean
make 
./bench $1 15 >> eval_size.txt
sleep 10
./bench $1 13 >> eval_size.txt
sleep 10
./bench $1 16 >> eval_size.txt
sleep 10
./bench $1 13 >> eval_size.txt
sleep 10