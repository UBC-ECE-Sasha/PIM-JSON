#!/bin/bash
echo " " > eval_size.txt

for i in  100 60120 125 128 130 140 145 150 160 
do
  echo "#### $i start ####" >> eval_size.txt
  ./eval_size_sub.sh ../../../tweet_$1.json 2 16 $i
  echo " " >> eval_size.txt
  echo " " >> eval_size.txt
  echo "$i done"
done
