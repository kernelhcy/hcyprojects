/* 
 * File:   main.cpp
 * Author: hcy
 *
 */

#include "headers/FileActions.h"
#include "headers/LexicalAnalysis.h"
#include "headers/SyntaxAnalysis.h"

//using namespace std;

/*
 * 
 */
int main(int argc, char** argv)
{

	if (argc != 3)
	{
		std::cerr << "Bad Paremeters!\n";

		exit(1);
	}

	std::string in_file(argv[1]);
	std::string out_file(argv[2]);
	SyntaxAnalysis *sa = SyntaxAnalysis::get_instance(in_file, out_file);
	sa->analysis();


//	char buffer[500];
//	std::ifstream ifs;
//	ifs.open("in", std::ios::in | std::ios::out);
//	while (!ifs.eof())
//	{
//		ifs.getline(buffer, 500);
//		std::cout << buffer;
//	}

	return (EXIT_SUCCESS);
}

