#include "headers.h"
#include "fs.h"

int init();

/*
 * 程序入口
 */
void run(bool show_details)
{
	system("clear");//清屏

	std::cout << "HCY's Fils System.\n";
	std::cout << "Version 0.001. \n";
	
	init();

	std::cout << "Input \"help or h\" for help infromation.\n";
	
	std::string cmd;

	while(true)
	{
		std::cout <<"cmd:";
		std::cin >> cmd;
		
		if(cmd == "quit" || cmd == "q")
		{
			break;
		}
		else if(cmd == "help" || cmd == "h")
		{
			show_help_info();
		}
		else if(cmd == "ls")
		{
			ls();
		}
		else if(cmd == "mkdir")
		{
			mkdir();
		}
		else if(cmd == "rmdir")
		{
			rmdir();
		}
		else if(cmd == "chdir" || cmd == "cd")
		{
			chdir();
		}
		else if(cmd == "create")
		{
			create_f();
		}
		else if(cmd == "delete")
		{
			delete_f();
		}
		else if(cmd == "open")
		{
			open_f();
		}
		else if(cmd == "close")
		{
			close_f();
		}
		else if(cmd == "write")
		{
			write_f();
		}
		else if(cmd == "read")
		{
			read_f();
		}
		else if(cmd == "pwd")
		{
			pwd();
		}
		else if(cmd == "login")
		{
			login();
		}
		else if(cmd == "logout")
		{
			logout();
		}
		else if(cmd == "cat")
		{
			cat();
		}
		else
		{
			std::cout << "No such command!\nType \"help or h\" for help information.\n";
			continue;
		}

	}


	if(show_details)
	{
		std::cout << "Show details.\n";
	}
}

/*
 * 初始化文件系统
 */
int init()
{
	std::cout << "初始化文件系统...\n";
	return 0;
}

/************************************/

int ls()
{
	std::cout << "ls\n";
	return 0;
}
int mkdir()
{
	std::cout << "mkdir\n";
	return 0;
}
int rmdir()
{
	std::cout << "rmdir\n";
	return 0;
}
int chdir()
{
	std::cout << "chdir\n";
	return 0;
}
int create_f()
{
	std::cout << "create\n";
	return 0;
}
int delete_f()
{
	std::cout << "delete\n";
	return 0;
}
int open_f()
{
	std::cout << "open\n";
	return 0;
}
int close_f()
{
	std::cout << "close\n";
	return 0;
}
int write_f()
{
	std::cout << "write\n";
	return 0;
}
int read_f()
{
	std::cout << "read\n";
	return 0;
}
int pwd()
{
	std::cout << "pwd\n";
	return 0;
}
int login()
{
	std::cout << "login\n";
	return 0;
}
int logout()
{
	std::cout <<"logout\n";
	return 0;
}
int cat()
{
	std::cout <<"cat\n";
	return 0;
}
void show_help_info()
{
	std::cout << "ls mkdir rmdir chdir create delete open close write read quit login logout pwd cat\n";
}

