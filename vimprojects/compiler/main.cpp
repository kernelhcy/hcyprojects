/* 
 * File:   main.cpp
 * Author: hcy
 *
 */

#include "headers/FileAction.h"
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
    
    std::string in_file_name(argv[1]);
    std::string out_file_name(argv[2]);

    std::ifstream ifs;
    std::ofstream ofs;
    FileAction fa(in_file_name, out_file_name, ifs, ofs);
    LexicalAnalysis la(fa);

    //la.analysis();

    std::cout << "Being...\n";
    SyntaxAnalysis sa(la);
    int status = sa.analysis();
    sa.~SyntaxAnalysis();

    std::cout << "Over!!\nStatus:" << status << std::endl;

    exit(0);
    //return (EXIT_SUCCESS);
}


