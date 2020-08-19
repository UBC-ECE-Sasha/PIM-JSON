#!/bin/bash
echo " " > eval_size.txt
echo "#### 64 start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_4GB.json 2 16 64
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "64 done"
echo "#### 128 start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_4GB.json 2 16 128
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "128 done"
echo "#### 192 start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_4GB.json 2 16 192
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "192 done"
echo "#### 256 start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_4GB.json 2 16 256
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "256 done"
echo "#### 320 start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_4GB.json 2 16 320
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "320 done"
echo "#### 384 start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_4GB.json 2 16 384
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "384 done"
echo "#### 448 start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_4GB.json 2 16 448
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "448 done"
echo "#### 512 start ####" >> eval_size.txt
./eval_size_sub.sh ../../../tweet_4GB.json 2 16 512
echo " " >> eval_size.txt
echo " " >> eval_size.txt
echo "512 done"
