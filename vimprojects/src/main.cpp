#include <iostream>
#include <cstdlib>
#include "textquery.h"

std::ifstream& open_file(std::ifstream &in, const std::string &file)
{
	in.close(); 		 	//close in case it was already open
	in.clear(); 			//clear any exisiting errors
	in.open(file.c_str()); 	//if the open fails, the stream will be in an invalid state
	return in; 				//condition sate is good if open succeeded
}

int main(int argv, char *argc[])
{
	std::ifstream infile;
	if (argv < 2 || !open_file(infile, argc[1]))
	{
		std::cerr << "No input file!\n";
		return EXIT_FAILURE;
	}

	TextQuery tq;	
	tq.read_file(infile);
	tq.print_result();
	return EXIT_SUCCESS;
}

