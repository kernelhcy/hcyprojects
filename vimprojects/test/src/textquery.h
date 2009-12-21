#ifndef _TEXTQUERY_H
#define _TEXTQUERY_H

#include <vector>
#include <set>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

class TextQuery
{
	public:
		//typedef to make declarartion easier
		typedef std::vector<std::string>::size_type line_no;
		
		TextQuery();
		TextQuery(const std::string&);
		~TextQuery();

		/*
		 * interface:
		 * 		read_file builds the insternal data structure for the given file
		 * 		run_query finds the given word and returns set of lines on which it appears
		 * 		text_line returns a requested line from the input file
		 */
		void read_file(std::ifstream &is)
		{
			store_file(is);
			build_map();
		}
		std::set<line_no> run_query(const std::string&) const;
		std::string text_line(line_no) const;
		void print_result();
	private:
		//utility functions used by read_file
		//store input file
		void store_file(std::ifstream&); 	

		//associated each word with a set of line numbers
		void build_map();

		//remober the whole input file
		std::map< std::string, std::set<line_no> > word_map;

		//the file name of the text
		std::string filename;
		//store the text of the file.
		std::vector<std::string> lines_of_text;
}; 

#endif
