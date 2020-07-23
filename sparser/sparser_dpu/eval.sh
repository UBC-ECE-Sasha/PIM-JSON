#!/bin/bash

# $1 filename $2 number of tasklets $3 number of DPUs
sed -i '4d' Makefile
sed -i "4 i NR_TASKLETS\ ?=\ $2" Makefile
sed -i '5d' Makefile
sed -i "5 i NR_DPUS\ ?=\ $3" Makefile
sed -i '6d' Makefile
sed -i "6 i QUERY\ ?=\ $4" Makefile
make clean
make 
./bench $1