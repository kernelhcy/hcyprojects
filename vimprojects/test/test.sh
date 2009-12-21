#! /bin/bash

FROMDIR="/home/hcy"
TODIR="/media/移动硬盘"
TARGET=$1

#递归的处理目录
function dodir
{
	#目录不存在，创建
	if [ ! -e $TODIR"/"$1 ];then
		mkdir $TODIR"/"$1
	fi

	#读取目录的内容
	lists=`ls -A $FROMDIR"/"$1`

	#逐条处理目录项
	for a in $lists
	do
		#得到下一级目录，递归处理之
		if [ -d $FROMDIR"/"$1"/"$a ];then
			dodir $1"/"$a
		else
			#判断文件在目的位置是否存在，
			#不存在，则进行复制
			if [ ! -e $TODIR"/"$1"/"$a ];then
				echo $FROMDIR"/"$1"/"$a
				cp $FROMDIR"/"$1"/"$a $TODIR"/"$1"/"$a
			fi
		fi
	done
}

#程序入口
if [ ! -e $TODIR"/"$TARGET ];then
	mkdir $TODIR"/"$TARGET
fi

a=`ls -A  $FROMDIR"/"$TARGET`
for s in $a
do
	if [ -d $FROMDIR"/"$TARGET"/"$s ]; then
		dodir $TARGET"/"$s
	else
		if [ ! -e $TODIR"/"$TARGET"/"$s ];then
			echo $FROMDIR"/"$TARGET"/"$s
			cp $FROMDIR"/"$TARGET"/"$s $TODIR"/"$TARGET"/"$s
		fi
	fi

done

