/* 
 * File:   LexicalAnalysis.h
 * Author: hcy
 *
 */

#ifndef _LEXICALANALYSIS_H
#define	_LEXICALANALYSIS_H

#include "FileActions.h"
#include "headers.h"

/*
 *  The class LexicalAnalysis is to analysis the accidence of the source file.
 *
 *  The id of the keyword,label,constant and do on is defined below:
 *         +-------------+----------+
 *         | Item        |    Id    |
 *         +-------------+----------+
 *         | main        |    1     |
 *         | {           |    2     |
 *         | }           |    3     |
 *         | bool        |    4     |
 *         | int         |    5     |
 *         | if          |    6     |
 *         | return      |    7     |
 *         | else        |    8     |
 *         | do*         |    9     |
 *         | while       |    10    |
 *         | false       |    11    |
 *         | true        |    12    |
 *         |    +        |    13    |
 *         |    -        |    14    |
 *         |    *        |    15    |
 *         |    /        |    16    |
 *         |    (        |    17    |
 *         |    )        |    18    |
 *         |    =        |    19    |
 *         |    >        |    20    |
 *         |    <        |    21    |
 *         |    ;        |    22    |
 *         |    ==       |    23    |
 *         |    !=       |    24    |
 *         |    ,        |    25    |
 *         |    &&       |    26    |
 *         |    ||       |    27    |
 *         |    !        |    28    |
 *         |    / *      |    29    |
 *         |    * /      |    30    |
 *         |    //       |    31    |
 *         | constants   |    32    |
 *         |  labels     |    33    |
 *         +-------------+----------+
 *
 *  The index of the key word add one in the vector is the id of this key word;
 */
class LexicalAnalysis
{
	public:

		static LexicalAnalysis* get_instance(const std::string& in_path,
				const std::string& out_path);
		/*
		 * get the already existed instance.
		 * it will return NULL, if there is no instance!
		 */
		static LexicalAnalysis* get_instance(void);
		~LexicalAnalysis(void);
		/*
		 *  Do lexical analysis on the string in the buffer
		 *
		 */
		void analysis(void);

		/*
		 * Get the next word of the sentence.
		 * Used by the syntax analysis program.
		 *
		 */
		int get_next_word(void);

		/*
		 * get the position of the labels or constants in
		 * 	label table or constant table.
		 */
		int get_pos(void);

		/*
		 * get the word from the id
		 */
		std::string& get_string(int id);
		/*
		 *  Get the table of the words;
		 */
		std::vector<std::string>* get_label_table(void);

		/*
		 *  Get the table of the constant.
		 */
		std::vector<int>* get_const_table(void);
		/*
		 * show the syntax error
		 * info : the information of the error
		 */
		void error(const std::string &info);

	private:
		//store the character readed from the buffer
		char ch;
		//the position of the lable or constant in label table or constant table
		int pos;
		//store the sting which makes up a word
		std::string str_token;

		//the table of labels
		std::vector<std::string> label_table;

		//the table of constants
		std::vector<int> const_table;

		//store the key words of this language
		std::vector<std::string> keywords;

		std::vector<std::string> id_string;

		/*
		 *  Read from the source file
		 */
		FileAction* fa;

		/*
		 * singleton
		 */
		static LexicalAnalysis * _instance;

		/*
		 *  Out put the result to the output file
		 */
		void output(int id, const char *entry);

		/*
		 *  Get a character from the buffer and put the character into ch.
		 *  The index of the buffer goes ahead.
		 *  Return value: the character
		 */
		char get_char(char &ch);

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
		void concat(void);

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
		int reserve(void);

		/*
		 *  Put the character in ch back into the buffer,and set ' ' into ch.
		 */
		void retract(void);

		/*
		 *  Insert the sting in str_token into the label_table and return the pointer.
		 */
		int insert_id(void);

		/*
		 *  Insert the constant in str_token into the const_table and return the pointer.
		 */
		int insert_const(void);

		LexicalAnalysis(const std::string& in_path, const std::string& out_path);

		/*
		 * print the information
		 * mode :	0  standard output
		 * 			1  error output
		 */
		void print_info(const std::string &s, int mode);

};

#endif	/* _LEXICALANALYSIS_H */

