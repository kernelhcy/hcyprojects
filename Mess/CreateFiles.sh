#! /bin/sh
# 
# File:   CreateFiles.sh
# Author: hcy
#
# Created on 2009年5月11日, 上午11:23
#

if [ $# -lt "1" ];then
	echo "Plese input the number of files to create!"
	exit
fi

#创建的文件个数。
num=$1
echo "create $num files."

cnt="1"
entry="#include <stdlib.h>\nint main()\n{\n\treturn 0;\n}\n"

#echo -e  $entry
while [ $cnt -lt $num ]
do
	touch $cnt.c
	cnt=$[$cnt+1]
	echo -e $entry>>$cnt.c
done
