#!/bin/bash
echo " " > eval_cost_yelp.txt
echo "yelp_64MB.json start" >>  eval_cost_yelp.txt
./eval_COST_yelp.sh yelp_64MB.json
echo "-------------------------------" >> eval_cost_yelp.txt
echo "yelp_256MB.json start" >>  eval_cost_yelp.txt
./eval_COST_yelp.sh yelp_256MB.json
echo "-------------------------------" >> eval_cost_yelp.txt
echo "yelp_400MB.json start" >>  eval_cost_yelp.txt
./eval_COST_yelp.sh yelp_400MB.json
echo "-------------------------------" >> eval_cost_yelp.txt
echo "yelp_640MB.json start" >>  eval_cost_yelp.txt
./eval_COST_yelp.sh yelp_640MB.json