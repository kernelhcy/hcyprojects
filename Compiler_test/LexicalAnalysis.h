/* 
 * File:   LexicalAnalysis.h
 * Author: hcy
 *
 */

#ifndef _LEXICALANALYSIS_H
#define	_LEXICALANALYSIS_H

#include "FileActions.h"
#include <stdlib.h>
#include <iostream>
#include "LexicalAnalysis.h"
#include <list>
#include <string>
#include <vector>


/*
 *  The class LexicalAnalysis is to analysis the accidence of the source file.
 *
 *  The id of the keyword,label,constant and do on is defined below:
 *         +-------------+----------+
 *         | Item        |    Id    |
 *         +-------------+----------+
 *         | program     |    1     |
 *         | begin       |    2     |
 *         | end         |    3     |
 *         | var         |    4     |
 *         | integer     |    5     |
 *         | if          |    6     |
 *         | then        |    7     |
 *         | else        |    8     |
 *         | do          |    9     |
 *         | while       |    10    |
 *         | constant    |    11    |
 *         | labels      |    12    |
 *         |    +        |    13    |
 *         |    _        |    14    |
 *         |    (        |    15    |
 *         |    )        |    16    |
 *         |    =        |    17    |
 *         |    >        |    18    |
 *         |    <        |    19    |
 *         |    ;        |    20    |
 *         |    :        |    21    |
 *         |    :=       |    22    |
 *         |    ,        |    23    |
 *         +-------------+----------+
 *
 *  The index of the key word add one in the vector is the id of this key word;
 */
class LexicalAnalysis
{
public:

    LexicalAnalysis(FileAction& fa_p);
    ~LexicalAnalysis();
    /*
     *  Do lexical analysis on the string in the buffer
     *
     */
    void analysis();

   
    /*
     *  Get the size of the buffer.
     */
    int get_buffer_size();

    /*
     *  Set the buffer size.
     *  This function will reallocate memory for the buffer.
     *  If reallocating memory faild,this funcion will report an error
     *      and the size of the buffer will not be changed and the buffer
     *      will also not be changed.
     *  Return value:
     *      1 : fail
     *      0 : success
     */
    int set_buffer_size(int size);


    /*
     *  Get the buffer.
     *  Used to fill the buffer with characters.
     *  If there is no character to get , return '\0'
     */
    char* get_buffer();

    /*
     *  Get the table of the words;
     */
    std::list<std::string>& get_label_table();

    /*
     *  Get the table of the constant.
     */
    std::list<std::string>& get_const_table();
    
private:
    //store the character readed from the buffer
    char ch;

    //store the sting which makes up a word
    std::string str_token;

    //the table of labels
    std::list<std::string> label_table;

    //the table of constants
    std::list<std::string> const_table;

    //buffer
    char* buffer;

    //the size of the buffer
    int buffer_size;

    //the current position of the buffer.
    int index;

    //the length of the string in the buffer now.
    int len;

    //store the key words of this language
    std::vector<std::string> keywords;

    

    /*
     *  Read from the source file
     */
    FileAction& fa;

    /*
     *  Fill the buffer.
     *  Return value:
     *      the number of the characters fill into the buffer.
     *  If return 0, the end of the file has been accessed.
     */
    int fill_buffer();

    /*
     *  Out put the result to the output file
     */
    void output(int id,const char *entry);

    /*
     *  Get a character from the buffer and put the character into ch.
     *  The index of the buffer goes ahead.
     *  Return value:
     *      1 : success to get a character
     *      0 : faild to get a character,maybe the buffer is empty
     */
    int get_char(char &ch);

    /*
     *  Check if the character in ch is black.
     *  If is,invoke get_char()until the character in ch is not black.
     *  Return value:
     *      1 : success to get a character
     *      0 : faild to get a character,maybe the buffer is empty
     */
    int get_bc(char &ch);

    /*
     *  Append ch to the end of str_token.
     */
    void concat();

    /*
     *  Check if the character in ch is a letter.
     */
    bool is_letter(char ch);

    /*
     *  Check if the character in ch is a number.
     */
    bool is_digit(char ch);

    /*
     *  Check the string in str_token is a keyword of this language.
     *  If is a keyword,return the id of the keyword,else return 0.
     */
    int reserve();

    /*
     *  Put the character in ch back into the buffer,and set ' ' into ch.
     */
    void retract();

    /*
     *  Insert the sting in str_token into the label_table and return the pointer.
     */
    int insert_id();

    /*
     *  Insert the constant in str_token into the const_table and return the pointer.
     */
    int insert_const();


};

#endif	/* _LEXICALANALYSIS_H */

