#!/bin/bash
echo "#### 64 start ####" >> eval_out.txt
./eval.sh ../../../yelp_academic_dataset_review.json 2 16 64
echo "#### 64 end ####" >> eval_out.txt
echo " " >> eval_out.txt 
echo " " >> eval_out.txt 
echo "64 done"
sleep 10
echo "#### 128 start ####" >> eval_out.txt
./eval.sh ../../../yelp_academic_dataset_review.json 2 16 128
echo " " >> eval_out.txt
echo " " >> eval_out.txt 
echo "128 done"
sleep 10
echo "#### 192 start ####" >> eval_out.txt
./eval.sh ../../../yelp_academic_dataset_review.json 2 16 192
echo "#### 192 end ####" >> eval_out.txt
echo " " >> eval_out.txt
echo " " >> eval_out.txt 
echo "192 done"
sleep 10
echo "#### 256 start ####" >> eval_out.txt
./eval.sh ../../../yelp_academic_dataset_review.json 2 16 256
echo "#### 256 end ####" >> eval_out.txt
echo " " >> eval_out.txt
echo " " >> eval_out.txt 
echo "256 done"
sleep 10
echo "#### 320 start ####" >> eval_out.txt
./eval.sh ../../../yelp_academic_dataset_review.json 2 16 320
echo "#### 320 end ####" >> eval_out.txt
echo " " >> eval_out.txt
echo " " >> eval_out.txt 
echo "320 done"
sleep 10
echo "#### 384 start ####" >> eval_out.txt
./eval.sh ../../../yelp_academic_dataset_review.json 2 16 384
echo "#### 384 end ####" >> eval_out.txt
echo " " >> eval_out.txt
echo " " >> eval_out.txt 
echo "384 done"
sleep 10
echo "#### 448 start ####" >> eval_out.txt
./eval.sh ../../../yelp_academic_dataset_review.json 2 16 448
echo "#### 448 end ####" >> eval_out.txt
echo " " >> eval_out.txt
echo " " >> eval_out.txt 
echo "448 done"
sleep 10
echo "#### 512 start ####" >> eval_out.txt
./eval.sh ../../../yelp_academic_dataset_review.json 2 16 512
echo "#### 512 end ####" >> eval_out.txt
echo " " >> eval_out.txt
echo " " >> eval_out.txt 
echo "512 done"
sleep 10
echo "#### 576 start ####" >> eval_out.txt
./eval.sh ../../../yelp_academic_dataset_review.json 2 16 576
echo "#### 576 end ####" >> eval_out.txt
echo " " >> eval_out.txt
echo " " >> eval_out.txt 
echo "576 done"


