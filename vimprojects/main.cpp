/* 
 * File:   main.cpp
 * Author: hcy
 *
 */

#include "headers.h"
using namespace std;


/*
 * 
 */
int main(int argc, char** argv)
{
	
	bool show_details = false;

	std::cout << "\nShow details,run: "<< argv[0] <<" details.\n";
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


