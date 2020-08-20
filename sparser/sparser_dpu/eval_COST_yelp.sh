#!/bin/bash
echo " " > eval_cost_yelp.txt
echo $1 >> eval_cost_yelp.txt
for i in 64 110 120 125 130 140 145 160 180 320 384 448 512
do
  echo "#### $i start ####" >> eval_cost_yelp.txt
  ./eval.sh $1 2 16 $i eval_cost_yelp.txt
  echo " " >> eval_cost_yelp.txt
  echo " " >> eval_cost_yelp.txt
  echo "$i done"
done