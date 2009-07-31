/* 
 * File:   main.cpp
 * Author: hcy
 *
 */

#include "headers.h"
#include "fs.h"

/*
 * 
 */
int main(int argc, char** argv)
{
	
	bool show_details = false;

	printf("\nShow details,run: %s details.\n",argv[0]);
	if(argc > 1)
	{
		if(strcmp("details",argv[1]) == 0)
		{
			show_details = true;	
		}
	}
//	system("clear");
   	run(show_details);
    //exit(0);
//	char buffer[100];
//	FILE_P * fp = NULL;

//	init();
//	login("hcy","123456");
	
//	ls_t(NULL);

//	mkdir_t("hcy");
//	chdir_t("/hcy");
//	fp = create_f("0",75);
//	strcpy(buffer, "Hello , this is file /hcy/0.");
//	write_f(fp, buffer, strlen(buffer));
//	close_f(fp);

//	mkdir_t("doc");
//	chdir_t("/hcy/doc");
//	fp = create_f("1",75);
//	strcpy(buffer, "Hello , this is file /hcy/doc/1.");
//	write_f(fp, buffer, strlen(buffer));
//	close_f(fp);
	
//	chdir_t("/hcy");

//	fp = create_f("2",75);
//	strcpy(buffer, "Hello , this is file /hcy/2.");
//	write_f(fp, buffer, strlen(buffer));
//	close_f(fp);

//	ls_t(NULL);

//	delete_f("/hcy/0");
//	ls_t(NULL);
	
//	fp = create_f("3",75);
//	strcpy(buffer, "Hello , this is file /hcy/3.");
//	write_f(fp, buffer, strlen(buffer));
//	close_f(fp);
	
//	ls_t(NULL);

//	fp = open_f("/hcy/2", R | W);
//	strcpy(buffer, "Hello , this is file /hcy/2.");
//	write_f(fp, buffer, strlen(buffer));
//	close_f(fp);

//	logout("hcy");
	//halt();
    return (EXIT_SUCCESS);
}


