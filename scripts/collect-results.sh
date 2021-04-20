#!/bin/bash

for f in ./*.out; do
	param=`echo $f | cut -d '-' -f 2`
	res_line=`grep 'max delay' $f`
	max=0
	while IFS= read -r line; do
		num=`echo $line | cut -d ' ' -f 6`
		((num > max)) && max=$num
	done <<< "$res_line"
	printf '%f  %f\n' $param $max
done
