#include "headers.h"
#include "digraph.h"
#include "out.h"
#include <unistd.h>

char *help_info = "\n用法： sfi [-h] [-s 源文件类型] [-o 输出文件名称] [-f 输出文件格式] 目录\n\n"
				  "\t-h\n"
				  "\t\t输出这个帮助信息。\n"
				  "\t-s\n"
				  "\t\t要分析的源文件的类型。包括：\n"
				  "\t\t\tcpp 	: c++源文件\n"
				  "\t\t\tc   	: c源文件\n"
				  "\t\t\tjava 	: java源文件\n"
				  "\t\t\tcsharp  : c#源文件\n"
				  "\t-o\n"
				  "\t\t分析结果输出的文件名。不指定此选项将默认保存为pic.jpg\n"
				  "\t-f\n"
				  "\t\t输出的文件格式。包括：\n"
				  "\t\t\ttext 	: 文本格式\n"
				  "\t\t\tjpg 	: jpg图片格式\n"
				  "\t\t\tpng 	: png图片格式\n"
				  "\n"
				  "如果包含-h，则直接输出帮助信息，其他选项都无效。bug报告：sfibug@126.com\n";

void show_help()
{	
	printf("%s\n", help_info);
	exit(0);
}


int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		show_help();
	}
	src_t type;
	type_t out_type = JPG_T;
	char out_filename[MAXFILENAMELEN] = "result.sfi";

	int c;
	while ((c = getopt(argc, argv, "hs:o:f:")) != -1)
	{
		switch (c)
		{
			case 'h':
				show_help();//exit program
			case 's':
				log_info("The source file type : %s\n", optarg);
				if (strcmp(optarg, "cpp") == 0)
				{
					type = CPP_T;
				}
				else if (strcmp(optarg, "c") == 0)
				{
					type = C_T;
				}
				else if (strcmp(optarg, "java") == 0)
				{
					type = JAVA_T;
				}
				else if (strcmp(optarg, "csharp") == 0)
				{
					type = CSHARP_T;
				}
				else
				{
					log_info("Unknown source file type!Plese run with -h.\n");
					exit(1);
				}
				break;
			case 'o':
				//log_info("Out put file name : %s\n", optarg);
				strcpy(out_filename, optarg);
				strcat(out_filename, ".sfi");
				break;
			case 'f':
				//log_info("Out put file type : %s\n", optarg);
				if (strcmp(optarg, "text") == 0)
				{
					out_type = TXT_T;
				}
				else if (strcmp(optarg, "jpg") == 0)
				{
					out_type = JPG_T;	
				}
				else if (strcmp(optarg, "png") == 0)
				{
					out_type = PNG_T;
				}
				else 
				{
					log_info("Unknown out type : %s \n", optarg);
					exit(1);
				}
				break;
			default:
				printf("Unknown option.\n");
				exit(1);
		}
	}

	//禁止检索根目录。
	if (strlen(argv[argc - 1]) == 1 && argv[argc - 1][0] == '/')
	{
		printf("Wrong dirctory!\n");
		exit(1);
	}

	digraph *dg = create_digraph(argv[argc - 1], CPP_T);
	/*
	//test	
	char *test[7] = {"aa", "bb", "cc", "dd", "ee", "ff", "gg"};

	digraph *dg = digraph_init();
	
	digraph_build_edge_string(dg, test[0], test[1]);
	digraph_build_edge_string(dg, test[0], test[2]);
	digraph_build_edge_string(dg, test[0], test[3]);
	digraph_build_edge_string(dg, test[0], test[4]);
	digraph_build_edge_string(dg, test[1], test[4]);
	digraph_build_edge_string(dg, test[1], test[3]);
	digraph_build_edge_string(dg, test[2], test[4]);
	digraph_build_edge_string(dg, test[3], test[1]);
	digraph_build_edge_string(dg, test[4], test[2]);
	digraph_build_edge_string(dg, test[5], test[3]);
	digraph_build_edge_string(dg, test[6], test[4]);
	digraph_build_edge_string(dg, test[6], test[3]);
	digraph_build_edge_string(dg, test[6], test[2]);
	*/

//	digraph_show(dg);	
	create_out(dg, out_type, out_filename);
	digraph_free(dg);
	

	return 0;
}
