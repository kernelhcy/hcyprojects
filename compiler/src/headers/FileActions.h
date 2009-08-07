/* 
 * File:   FileActions.h
 * Author: hcy
 * Define the defination of class FileActions
 */

#ifndef _FILEACTIONS_H
#define	_FILEACTIONS_H

#include "FileActions.h"
#include "headers.h"

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
		 * return the only instace of this class
		 *
		 * if no parameters,these paths are defaule.
		 *
		 * in_path : the path of the source file.
		 * out_path : the path of the output file.
		 */
		static FileAction * get_instance();
		static FileAction * get_instance(const std::string& in_path, const std::string& out_path);

		~FileAction(void);

		/*
		 *  get the path of the source file.
		 *
		 *  This function just return the pointer pointing to the class member 'path'.
		 *  So,DO NOT modify the entry pointed by it.
		 *
		 */

		const std::string& get_in_path(void);

		/*
		 *  get the path of the output file.
		 *
		 *  This function just return the pointer pointing to the class member 'path'.
		 *  So,DO NOT modify the entry pointed by it.
		 *
		 */

		const std::string& get_out_path(void);
		/*
		 * return the position of pointer.
		 * using in error display.
		 */
		int get_line(void);
		int get_colume(void);

		/*
		 *  Put the character in ch back into the buffer
		 *  and index--;
		 */
		void retract(void);

		/*
		 * fill the buffer with the next line
		 * if reach the end of the file return -1, else return 0
		 */
		int next_line(void);

		/*
		 *  get a character from the input stream
		 */
		char get_char(void);
	private:
		std::string in_path;//the path of the source file

		std::string out_path;//the path of the output file

		std::ifstream ifs;//file input stream

		std::ofstream ofs;//file output stream

		int line;//the line of current pointer

		char * buffer; //the buffer
		int buffer_size;//the size of the buffer
		int index;//the position of the pointer in the buffer
		int buffer_len;//the length of the buffer.

		/*
		 * the only instace of the class FileAction
		 */
		static FileAction * _instance;

		/*
		 * The constructor is defined to private.
		 * Using singleton
		 */
		FileAction();

		/*
		 * in_path : the path of the source file.
		 * out_path : the path of the output file.
		 */
		FileAction(const std::string& in_path, const std::string& out_path);

		/*
		 * open the files
		 *
		 * return value:
		 *      1 : success
		 *      0 : fail
		 */
		int open_file(void);

		/*
		 *  close the file.
		 *
		 *  return value:
		 *      1 : success
		 *      0 : fail
		 */
		int close_file(void);

		/*
		 * initialize
		 */
		void init(void);
		/*
		 * refill the buffer.
		 * return the length of the buffer entry
		 * if return -1, access the end of the input file
		 */
		int fill_buffer(void);
};

#endif	/* _FILEACTIONS_H */

