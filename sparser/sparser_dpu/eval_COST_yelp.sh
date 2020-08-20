#!/bin/bash
echo $1 >> eval_cost_yelp.txt
for i in 32 64 96 120 160 192 224 256 288 320 352 384 416 448 480 512
do
  echo "#### $i start ####" >> eval_cost_yelp.txt
  ./eval.sh $1 2 16 $i eval_cost_yelp.txt
  echo " " >> eval_cost_yelp.txt
  echo " " >> eval_cost_yelp.txt
  echo "$i done"
done