#!/bin/bash
echo " " > eval_size.txt
echo "#### 128 start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_128MB.json 2 16 512
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "128 done"
echo "#### 256 start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_256MB.json 2 16 512
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "256 done"
echo "#### 512 start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_512MB.json 2 16 512
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "512 done"
echo "#### 1024 start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_1GB.json 2 16 512
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "1024 done"
echo "#### 4096 start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_4GB.json 2 16 512
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "4096 done"
echo "#### 8GB start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_8GB.json 2 16 512
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "8GB done"
echo "#### 12GB start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_12GB.json 2 16 512
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "12GB done"
echo "#### 16GB start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_16GB.json 2 16 512
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "16GB done"
echo "#### 20GB start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_20GB.json 2 16 512
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "20GB done"
echo "#### 22GB start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_22GB.json 2 16 512
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "22GB done"
echo "#### 24GB start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_22GB.json 2 16 512
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "24GB done"
echo "#### 30GB start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_30GB.json 2 16 512
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "30GB done"
