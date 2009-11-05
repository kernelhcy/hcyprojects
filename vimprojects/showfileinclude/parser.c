#include "parser.h"
/**
 * 三个辅助函数
 * 用于解析指定文件路径filepath的文件。
 * 解析的结果按照字符串数组的形式存放在result中。
 * 返回的结果是字符串数组result的长度。
 *
 * 三个函数的作用是解析文件中包含的文件。
 * 
 * c和c++中解析 #include "xxx.h" 和 #include <xxx.h>
 * java中解析 	import xxx.xxx.xxx.*
 * csharp中解析 using xx.xx.xx
 *
 */
static int parse_java(const char *filepath, char **result);
static int parse_c_cpp(const char *filepath, char **result);
static int parse_csharp(const char *filepath, char **result);

/*
 * 存储目录下的所有文件的完整路径和名称。
 */
static char 	**files = NULL;
static int 		file_cnt = 0; 	//文件的个数，files数组的长度。
/**
 * 解析dirpath路径
 * 将改路径目录下的所有文件保存在files中。
 */
static int parse_dir(const char *dirpath);

/*
 * 返回文件的后缀名类型。
 * .h .c .cpp .java .cs
 */
static file_t get_file_type(const char *path);

/*
 * 定义源文件解析函数函数指针。
 */
typedef int (*parse_fun_p)(const char*, char **);
static parse_fun_p parse_fun = NULL;

//工作目录，通过参数传进来的目录
static const char *base_path;
/**
 * 根据dirpath指定的路径解析路径目录下的所有文件
 * t指定文件类型。
 * 返回有向图。
 */
digraph* create_digraph(const char* dirpath, src_t t)
{
	base_path = dirpath;
	switch (t)
	{
		case CPP_T:
		case C_T:
			parse_fun = parse_c_cpp;
			break;
		case JAVA_T:
			parse_fun = parse_java;
			break;
		case CSHARP_T:
			parse_fun = parse_csharp;
			break;
		default:
			log_err("Unknown src type. %s %d", __FILE__, __LINE__);
			exit(1);
	}
	int cnt = parse_dir(dirpath);
	printf("files cnt %d\n", cnt);
	return NULL;
}

static int parse_dir(const char *dirpath)
{
	DIR 			*dp;
	struct dirent 	*dirp;
	int cnt = 0;

	if ((dp = opendir(dirpath)) == NULL)
	{
		log_err("Open dir error : %s", dirpath);
		exit(1);
	}
	struct stat 	buf;
	char 			fullpath[200];

	while((dirp = readdir(dp)) != NULL)
	{
		//忽略隐藏目录和文件
		if (dirp -> d_name[0] == '.')
		{
			continue;
		}
		
		//拼接子文件的完整路径。
		strcpy(fullpath, dirpath);
		//适当的加上/
		if (fullpath[strlen(fullpath) - 1] != '/')
		{
			strcat(fullpath, "/");
		}
		strcat(fullpath, dirp -> d_name);

		//获得文件的状态，并判断其是否是目录
		if (stat(fullpath, &buf) < 0)
		{
			log_err("Get file state error : %s", fullpath);
		}
		if (S_ISDIR(buf.st_mode)) 	//是目录，递归的处理之。
		{
			cnt += parse_dir(fullpath);
			continue;
		}

		if (get_file_type(dirp -> d_name) == UNKNOWN_F_T)
		{
			continue;
		}

		printf("%s\n", fullpath);

		parse_fun(fullpath, NULL);	

		int i = strlen(base_path);
		int j = 0;
		while(fullpath[j++] = fullpath[i++]);
		fullpath[j] = '\0';
		printf("absulate path: %s\n", fullpath);

		++cnt;
	}

	closedir(dp);
	return cnt;
}
/**
 * 相同返回非0,不同返回0.
 */
int strcmp_n(const char *a, const char *b, int begin, int end)
{
	int a_len = strlen(a);
	int b_len = end - begin;

	if (a_len != b_len)
	{
		return 0;
	}
	
	int a_i = 0, b_i = begin;
	while (a_i < a_len && a[a_i++] == b[b_i++]);

	return a_i == a_len;
}

static file_t get_file_type(const char *path)
{
	size_t len = strlen(path);
	int end, begin;
	end = len;
	begin = len - 1;
	
	while (begin >=0 && path[begin] != '.')
	{
		--begin;
	}
	++begin;
	if (end - begin == 1)
	{
		if (path[begin] == 'c')
		{
			return C_F_T;
		}
		else if (path[begin] == 'h')
		{
			return H_F_T;
		}
	}

	if (end - begin == 3 && ( 
				strcmp_n("cpp", path, begin, end) != 0
				|| strcmp_n("cxx", path, begin, end) != 0 )
				)
	{
		return CPP_F_T;
	}

	if (end - begin == 4 && strcmp_n("java", path, begin, end) != 0)
	{
		return JAVA_F_T;
	}

	if (end - begin == 2 && strcmp_n("cs", path, begin, end) != 0)
	{
		return CS_F_T;
	}

	return UNKNOWN_F_T;

}
static int parse_java(const char *filepath, char **result)
{
	log_info("parse java.");
}
static int parse_c_cpp(const char *filepath, char **result)
{
	int 	buf_len = 500;
	char 	buf[500];

	FILE 	*fp;
	if ((fp = fopen(filepath, "r")) == NULL)
	{
		log_err("Can not open file : %s", filepath);
		exit(1);
	}

	int 	read_len;
	int 	index;
	int 	begin, end;
	char 	includepath[buf_len];
	int 	includeindex;
	while (fgets(buf, buf_len, fp) != NULL)
	{
		read_len = strlen(buf);
		/*
		 * 头文件是以#include "" 或者#include <>形式表示的。
		 * 如果此行的长度小于9就不可能存在include声明。
		 */
		if (read_len < 9)
		{
			continue;
		}

		//判断是否以#开头，若不是，继续读下一行。
		index = 0;
		while(buf[index] == ' ') 	//除去开头的空白。
		{
			++index;
		}
		if(buf[index] != '#')
		{
			continue;
		}
		
		while(buf[++index] == ' ');
		begin = index;
		end = begin + 7; //include 只有7个字母，因此只需比较7个字母是不是include即可。
		if (strcmp_n("include", buf, begin, end) == 0)
		{
			continue;
		}
		
		while (buf[begin] != '"' && buf[begin] != '<')
		{
			++begin;
		}
		++begin;
		end = begin;
		while (buf[end] != '"' && buf[end] != '>')
		{
			++end;
		}
		//remove "../" and "./" at the beginning of the buf
		while(buf[begin++] == '.');
		--begin;
		if(buf[begin] == '/')
		{
			++begin;
		}

		includeindex = 0;
		for (; begin < end; ++begin, ++includeindex)
		{
			includepath[includeindex] = buf[begin];
		}
		includepath[includeindex] = '\0';

		printf("\t%s\n", includepath);
	}
 
	fclose(fp);

}
static int parse_csharp(const char *filepath, char **result)
{
	log_info("parse csharp");
}


