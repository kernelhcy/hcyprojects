#! /bin/sh
# 
# File:   countCFiles.sh
# Author: hcy
#
# Created on 2009年5月11日, 上午11:23
#

echo $1
count()
{
	echo -n "Number of matcher for $1 : "
	local cnt=`ls $1 | wc -w`
	echo $cnt
};


#判断输入的参数
if [ $# -lt "1" ];then
	echo "Bad parameter!!"
	exit
fi

matcher="*.c"
count "$matcher"

