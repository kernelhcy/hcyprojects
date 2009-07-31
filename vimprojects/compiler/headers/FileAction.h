/* 
 * File:   FileActions.h
 * Author: hcy
 * Define the defination of class FileActions
 */

#ifndef _FILEACTIONS_H
#define	_FILEACTIONS_H

#include "FileAction.h"
#include <stdlib.h>
#include <iostream>



/*
 * This header file must be contained in this file!
 * Else the compiler will report error messages.
 *
 */
#include <fstream>

/**
 *
 * This class is in charge of dealing with the source file.
 * Its funcions include openning and closing file,
 *                      reading from the file
 *                      and filling the buffer with characters read from the source file.
 *
 * NOTE:
 * The input stream need to be carried from the outside of the class,
 *  which means this class does not contain a input file stream.
 * You need to create a input file stream by yourself and
 *  carry this stream into the class.
 *
 */
class FileAction
{
 
public:

    /*
     * The parameter ifs is a file input stream which is created by the creator
     *  of the instance of this class.
     *
     * This parameter is needed!
     *
     */
    FileAction(std::ifstream& ifs,std::ofstream& ofs);
    
    /*
     * The parameter 'path' is the path of the source file.
     * 'ifs' is the input file stream.
     *
     */
    FileAction(const std::string& in_path,const std::string& out_path
            ,std::ifstream& ifs,std::ofstream& ofs);

    ~FileAction();

    /*
     *  use the path to open the file
     *  return value:
     *          1 : success
     *          0 : fail
     */
    int open_file();

    /*
     *  Fill the buffer with 'size' characters.
     *  the noused black space while be deleted.
     *
     *  For example, the input string is "aaa   ccc".
     *  There are three blackspaces between "aaa" and "ccc".
     *  When putting this string into the buffer,only one space
     *  will be remained,which means "aaa ccc" will be put into
     *  the buffer.
     *
     *  return value:
     *      the number of characters that are fill into the buffer
     */
    int fill_buffer(char *buffer,int size);
    
    /*
     *  close the file.
     *
     *  NOTE:
     *  This function must be invoked at the end of the program.
     *  Because when deconstruct the instance of this class,no action will be
     *  tacken to close the openned files!
     *
     *  return value:
     *      1 : success
     *      0 : fail
     */
    int close_file();

    /*
     *  get the path of the source file.
     *
     *  This function just return the pointer pointing to the class member 'path'.
     *  So,DO NOT modify the entry pointed by it.
     *
     */
    
    const std::string& get_in_path();
    /*
     *  set the path of the source file.
     */
    void set_in_path(const std::string& path);

    /*
     *  get the path of the output file.
     *
     *  This function just return the pointer pointing to the class member 'path'.
     *  So,DO NOT modify the entry pointed by it.
     *
     */

    const std::string& get_out_path();
    /*
     *  set the path of the output file.
     */
    void set_out_path(const std::string& path);

    /*
     *  return the output file stream
     */
    std::ofstream& get_ofstrem();
private:
    std::string in_path;//the path of the source file

    std::string out_path;//the path of the output file

    std::ifstream& ifs;//file input stream

    std::ofstream& ofs;//file output stream
   
    /*
     *  get a character from the input stream
     */
    char get_char();
   
};


#endif	/* _FILEACTIONS_H */

