#ifndef _PARSER_H
#define _PARSER_H
/**
 * 解析文件并根据文件的包含关系生成有向图。
 *
 */
#include "headers.h"
#include "digraph.h"
#include <dirent.h>
#include <sys/stat.h>

/**
 * 根据指定的目录路径dirpath和源文件类型t生成有向图。
 */
digraph* create_digraph(const char* dirpath, src_t t);

/**
 * 定义文件类型。
 * 主要根据后缀名来判断。
 */
typedef enum 
{
	H_F_T, 			//*.h
	CPP_F_T, 		//*.cpp
	C_F_T, 			//*.c
	JAVA_F_T, 		//*.jave
	CS_F_T,			//*.cs
	UNKNOWN_F_T 	//unknown
}file_t;

#endif
