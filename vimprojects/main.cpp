/* 
 * File:   main.cpp
 * Author: hcy
 *
 */

#include "headers.h"

using namespace std;

void e_h()
{
    printf("Program exit .\n");
} 

void e_h1()
{
    printf("Program exit 1.\n");
}

void e_h2()
{
    printf("Program exit 2.\n");
}
/*
 * 
 */
int main(int argc, char** argv)
{

    //atexit(e_h);
    //atexit(e_h1);
    //atexit(e_h2);
    run();
    //exit(0);
    return (EXIT_SUCCESS);
}


