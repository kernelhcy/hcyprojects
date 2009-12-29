#ifndef _BASE_H
#define _BASE_H

/**
 * 定义支持分析的源文件的类型。
 * 其中的c文件和c++文件的处理相同，可以通用。
 */
typedef enum
{
	CPP_T, 		//c++文件 
	C_T, 		//c文件
	JAVA_T, 	//Java文件
	CSHARP_T	//C#文件
}src_t;


typedef enum
{
	TXT_T, 		//普通文本格式
	JPG_T, 		//jpg图片格式
	PNG_T, 		//png图片格式
	DOT_T 		//dot语言格式
}type_t;

//定义最长的文件名
#define MAXFILENAMELEN 200

#endif
