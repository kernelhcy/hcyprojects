/* 
 * File:   SyntaxAnalysis.h
 * Author: hcy
 *
 */

#ifndef _SYNTAXANALYSIS_H
#define	_SYNTAXANALYSIS_H

#include "LexicalAnalysis.h"
#include <stack>
/*
 * The syntax analysis class
 *
 */

class SyntaxAnalysis
{
public:
    /*
     * The parameter is the lexical analysis class.
     */
    SyntaxAnalysis(LexicalAnalysis &la);
    ~SyntaxAnalysis();
    /*
     *  Analysising
     *  The return value: 0 :success; 1:there are some syntax errors.
     */
    int analysis();
    
private:
    //The lexical analysis class
    LexicalAnalysis& la;
    /*
     * Store the constant strings that used by
     *  the program.
     */
    std::string table1[16];

    //The size of some tables
    const static int SIZE = 26;

    //The predicrion analysis table
    int table2[SIZE][SIZE];

    //The stack.It's a char stack.
    std::stack<char> s;

    //
    std::string string_id[SIZE];
    
    //The information of the sentences
    int info;

    /*
     * Judge the word of the id is key word or not.
     */
    bool is_finalword(int id);

    /*
     * Push the string into the char stack reverse.
     */
    void push_all(const std::string &str , bool flag);

    /*
     * Get the id of the word.
     */
    int get_id(const std::string &str);

    /*
     * Print the content of the stack.
     */
    void print_stack();

    /*
     * Print the information of the sentence.
     */
    void print_info();

    /*
     * Print the position of the error.
     */
    void print_error_position();
};

#endif	/* _SYNTAXANALYSIS_H */

