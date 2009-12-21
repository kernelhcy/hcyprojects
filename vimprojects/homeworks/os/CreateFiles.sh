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

#创建目录
mkdir ./my
#更改工作目录
cd ./my

while [ $cnt -lt $num ]
do
	echo -e $entry>>f$cnt.c
	cnt=$[$cnt+1]
done
