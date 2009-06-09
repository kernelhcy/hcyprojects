/* 
 * File:   main.cpp
 * Author: hcy
 *
 */

#include "headers.h"


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

    run(show_details);
    //exit(0);
    return (EXIT_SUCCESS);
}


