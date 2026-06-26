#!/bin/bash
echo "Strategy comparison"
echo "-------------------"
for strategy in first_fit best_fit seg_list; do
    make STRATEGY=$strategy bench
    ./bench
done