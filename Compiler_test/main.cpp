/* 
 * File:   main.cpp
 * Author: hcy
 *
 */

#include "FileActions.h"
#include <stdlib.h>
#include <iostream>
#include "LexicalAnalysis.h"
#include "SyntaxAnalysis.h"
#include <string>

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
//    la.analysis();

    SyntaxAnalysis sa(la);
    sa.analysis();
    sa.~SyntaxAnalysis();

    std::cout<<"Over!!\n";

    exit(0);
    //return (EXIT_SUCCESS);
}


