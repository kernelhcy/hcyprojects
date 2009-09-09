/* 
 * File:   SyntaxAnalysis.h
 * Author: hcy
 *
 */

#ifndef _SYNTAXANALYSIS_H
#define	_SYNTAXANALYSIS_H

#include "LexicalAnalysis.h"
#include "CreateMidcode.h"
#include "headers.h"

/*
 * The syntax analysis class
 *
 */

class SyntaxAnalysis
{
public:


	static SyntaxAnalysis * get_instance(const std::string& in_path,
			const std::string& out_path);
	/*
	 * only use to get the already existed instance.
	 * if there is no instance of this class,
	 * this will return NULL.
	 */
	static SyntaxAnalysis * get_instance();
    ~SyntaxAnalysis(void);
    /*
     *  Analysising
     *  The return value: 0 :success; 1:there are some syntax errors.
     */
    int analysis(void);
private:
    DataTables *tables ;
    //The only instance of lexical analysis class
    LexicalAnalysis* la;

    //The only instance of file action class
    FileAction *fa;

    //The only instance of create middle code class
    CreateMidcode *cm;

    /*
     * the only instance
     */
    static SyntaxAnalysis * _instance;
    /*
     * Store the constant strings that used by
     *  the program.
     */
    std::string table1[50];

    //The size of some tables
    const static int SIZE = 50;

    //The predicrion analysis table
    int table2[SIZE][SIZE];

    //The stack.It's a char stack.
    std::stack<char> s;

    //
    std::string string_id[SIZE];

    //the scope of the current position
    int scope;

    /*
     * Judge the word of the id is key word or not.
     */
    bool is_finalword(int id);

    /*
     * Push the string into the char stack reverse.
     */
    void push_all(const std::string &str);

    /*
     * Get the id of the word.
     */
    int get_id(const std::string &str);

    /*
     * Print the content of the stack.
     */
    void print_stack();

    /*
     * Print the position of the error.
     */
    void print_error(const std::string &info);
    /*
     * singleton
     */
    SyntaxAnalysis(const std::string& in_path,const std::string& out_path);

    /*
     * print the information
     * mode:	1 normal information
     * 			0 error information
     */
    void print_info(const std::string &s, int mode);
};

#endif	/* _SYNTAXANALYSIS_H */

