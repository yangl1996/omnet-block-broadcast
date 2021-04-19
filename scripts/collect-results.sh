#!/bin/bash

for f in ./*.out; do
	param=`echo $f | cut -d '-' -f 2`
	res_line=`cat $f | grep 'Node 0 max delay'`
	res=`echo $res_line | cut -d ' ' -f 6 `
	printf '%f  %f\n' $param $res

done
