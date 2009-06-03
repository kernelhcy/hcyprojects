#! /bin/sh
# 
# File:   countCFiles.sh
# Author: hcy
# 用于统计目录my下的指定类型的文件个数。
# 例如：统计c文件的个数，输入：bash countCFiles.sh c
# Created on 2009年5月11日, 上午11:23
#

#统计文件的函数
count()
{
	#更改工作目录
	cd ./my
	echo -n "Number of matcher for $1 : "
    #创建临时变量，统计并记录文件个数。	
    local cnt=`ls $1 | wc -w`
    #输出文件个数
	echo $cnt
};


#判断输入的参数是否合法
if [ $# -lt "1" ];then
	echo "Bad parameter!!"
	exit
fi

#程序入口
count "$1"

