#!/usr/bin/bash

a=1
while [ $a -lt 99999 ]
do
	./main /home &
	a=$[$a+1]
done
