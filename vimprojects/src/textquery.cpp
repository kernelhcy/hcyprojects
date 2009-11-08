#include "textquery.h"

TextQuery::TextQuery()
{
	std::cout << "Please set the file name.\n";
}

TextQuery::TextQuery(const std::string &_filename)
	:filename(_filename)
{
	std::cout << "The file name is :" << filename << std::endl;
}

TextQuery::~TextQuery()
{
	std::cout << "Delete the TextQuery object.\n";
}

void TextQuery::store_file(std::ifstream &is)
{
	std::string textline;
	while (getline(is,textline))
	{
		lines_of_text.push_back(textline);
	}
}

//find whitespace-separeted words in the input vector
//add puts the word in word_map along with th eline number
void TextQuery::build_map()
{
	// process each line from input vector
	for (line_no line_num = 0; line_num != lines_of_text.size(); ++line_num)
	{
		//we'll use line to read the text a word at a time
		std::istringstream line(lines_of_text[line_num]);
		std::string word;
		while(line >> word)
		{
			//add this number to the set
			//subscript will add word to the map if it's not already there
			word_map[word].insert(line_num);
		}
	}
}

std::set<TextQuery::line_no> TextQuery::run_query(const std::string &query_word) const
{
	//Note: must use find and not subscript the map directly
	//to avoid adding words to word_map!
	std::map<std::string, std::set<line_no> >::const_iterator
					loc = word_map.find(query_word);
	if (loc == word_map.end())
	{
		return std::set<line_no>(); 	//not found, return empty set
	}
	else
	{
		//fetch and return set of line numbers for this word
		return loc -> second;
	}
}

std::string TextQuery::text_line(line_no line)const
{
	if (line < lines_of_text.size())
	{
		return lines_of_text[line];
	}

	//throw std::out_of_range("line number out of range.");
	return std::string("line number out of range");
}

void TextQuery::print_result()
{
	typedef std::set<line_no> line_nums;
	typedef std::map<std::string, std::set<line_no> > wordmap;
	
	line_nums::size_type size;
	wordmap::const_iterator ci = word_map.begin();
	while (ci != word_map.end())
	{
		size = ci -> second.size();
		std::cout << "\"" << ci -> first << "\" occurs "
			<< size << " time" << (size > 1 ? "s:\n" : ":\n");
		line_nums::const_iterator it = ci -> second.begin();
		while (it != ci -> second.end())
		{
			std::cout << "\t(line "
				      << (*it) + 1 << ") "
					  << lines_of_text[*it]
					  << std::endl;
			++it;
		}

		++ci;
		std::cout << std::endl;
	}

}
