#!/bin/bash
echo " " > eval_size.txt

for i in  128 192 256 320 384 448 512
do
  echo "#### $i start ####" >> eval_size.txt
  ./eval_size_sub.sh ../../../tweet_$1.json 2 16 $i
  echo " " >> eval_size.txt
  echo " " >> eval_size.txt
  echo "$i done"
done