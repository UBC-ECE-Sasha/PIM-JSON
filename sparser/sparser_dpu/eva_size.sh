#!/bin/bash
echo "#### 128 start ####" >> eval_size.txt
./eval.sh ../../../tweet_128MB.json 2 16 64
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "128 done"
echo "#### 256 start ####" >> eval_size.txt
./eval.sh ../../../tweet_256MB.json 2 16 128
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "256 done"
echo "#### 512 start ####" >> eval_size.txt
./eval.sh ../../../tweet_512MB.json 2 16 256
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "512 done"
echo "#### 1024 start ####" >> eval_size.txt
./eval.sh ../../../tweet_1GB.json 2 16 512
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "1024 done"
echo "#### 2048 start ####" >> eval_size.txt
./eval.sh ../../../tweet_2Gb.json 2 16 512
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "2048 done"