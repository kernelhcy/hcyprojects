#include <iostream>

#include "../headers/LexicalAnalysis.h"

LexicalAnalysis* LexicalAnalysis::_instance = NULL;

LexicalAnalysis* LexicalAnalysis::get_instance(const std::string& in_path,
		const std::string& out_path)
{
	if (_instance == NULL)
	{
		_instance = new LexicalAnalysis(in_path, out_path);
	}

	return _instance;
}

LexicalAnalysis::LexicalAnalysis(const std::string& in_path, const std::string& out_path)
{
	//std::cout << "The Lexical Analysis" << std::endl;
	ch = ' ';
	//get the instance of the FileAction
	fa = FileAction::get_instance(in_path, out_path);

	str_token.clear();

	//prepare the key words table
	/*
	 *  The index of the key word add one in the vector is the id of this key word;
	 *  For example, 'program''s id is 1.The id of 'then' is 7.
	 *                //key word    //Id
	 */
	keywords.push_back("main"); //1
	keywords.push_back("{"); //2
	keywords.push_back("}"); //3
	keywords.push_back("bool"); //4
	keywords.push_back("int"); //5
	keywords.push_back("if"); //6
	keywords.push_back("return"); //7
	keywords.push_back("else"); //8
	keywords.push_back("do"); //9 no use!
	keywords.push_back("while"); //10
	keywords.push_back("false"); //11
	keywords.push_back("true"); //12

}

LexicalAnalysis::~LexicalAnalysis()
{
	fa->~FileAction();
	str_token.~basic_string();

}

int LexicalAnalysis::get_next_word()
{
	int code;
	int value;

	int con;

	ch = ' ';
	str_token.clear();

	if (get_char(ch) == 0)
	{
		fa->~FileAction();
		return -1;
	};

	if (get_bc(ch) == 0)
	{
		fa->~FileAction();
		return -1;
	};

	if (is_letter(ch))
	{
		//Get a word!
		while (is_digit(ch) || is_letter(ch))
		{
			concat();
			con = get_char(ch);

			if (con == '\0')
			{
				break;
			}
		}

		retract();
		code = reserve();
		/*
		 * By default, zero is not the id of any key word!
		 */
		if (code == 0)
		{
			//Get a label!
			value = insert_id();
			output(33, str_token.data());
			code = 33;
		}
		else
		{
			output(code, str_token.data());
		}

		/*
		 *  The buffer is empty which means that the source file has no entry.
		 *  Job is done!
		 *  Exit.
		 */
		if (con == '\0')
		{
			fa->~FileAction();
			return -1;
		}

	}
	else if (is_digit(ch))
	{
		//Got a constant!
		while (is_digit(ch))
		{
			concat();
			con = get_char(ch);

			if (con == '\0')
			{
				break;
			}
		}
		retract();
		value = insert_const();
		output(32, str_token.data());
		code = 32;
		//Done
		if (con == '\0')
		{
			fa->~FileAction();
			exit(0);
		}
	}
	else if (ch == '+')
	{
		output(13, "+");
		code = 13;
	}
	else if (ch == '-')
	{
		output(14, "-");
		code = 14;
	}
	else if (ch == '*')
	{
		output(15, "*");
		code = 15;
	}
	else if (ch == '(')
	{
		output(17, "(");
		code = 17;
	}
	else if (ch == ')')
	{
		output(18, ")");
		code = 18;
	}
	else if (ch == '<')
	{
		output(21, "<");
		code = 21;
	}
	else if (ch == '>')
	{
		output(20, ">");
		code = 20;
	}
	else if (ch == ';')
	{
		output(22, ";");
		code = 22;
	}
	else if (ch == ',')
	{
		output(25, ",");
		code = 25;
	}
	else if (ch == '=')
	{
		con = get_char(ch);

		if (ch == '=')
		{
			output(23, "==");
			code = 23;
		}
		else
		{
			retract();
			output(19, "=");
			code = 19;
		}
	}
	else if (ch == '{')
	{
		output(2, "{");
		code = 2;
	}
	else if (ch == '}')
	{
		output(3, "}");
		code = 3;
	}
	else if (ch == '&')
	{
		con = get_char(ch);

		if (ch == '&')
		{
			output(26, "&&");
			code = 26;
		}
		else
		{
			error("need &&.");
			retract();

		}
		//Done
		if (con == '\0')
		{
			fa->~FileAction();
			return -1;
		}
	}
	else if (ch == '!')
	{
		con = get_char(ch);

		if (ch == '=')
		{
			output(24, "!=");
			code = 24;
		}
		else
		{
			output(28, "!");
			code = 28;
			retract();

		}
		//Done
		if (con == '\0')
		{
			fa->~FileAction();
			return -1;
		}
	}
	else if (ch == '|')
	{
		con = get_char(ch);

		if (ch == '|')
		{
			output(27, "||");
			code = 27;
		}
		else
		{
			retract();

		}
		//Done
		if (con == '\0')
		{
			fa->~FileAction();
			return -1;
		}
	}
	else if (ch == '/')
	{
		con = get_char(ch);

		if (ch == '/')
		{
			output(31, "//");
			code = 31;
			//next line.delete the  comments
			fa->next_line();
			/*
			 * 递归调用。
			 * 读取下一个非注释字符。
			 */
			return get_next_word();
		}
		else if (ch == '*')
		{
			output(29, "/*");
			code = 29;

			/*
			 * delete the comments
			 */
			char c1 = get_char(ch), c2 = get_char(ch);
			while (true)
			{
				c1 = c2;
				c2 = get_char(ch);
				if (c2 == '\0')
				{
					error("comments error");
					break;
				}

				if (c1 == '*' && c2 == '/')
				{
					output(30, "*/");
					code = 30;
					/*
					 * 递归调用。
					 * 获得下一个非注释字符。
					 */
					return get_next_word();
				}
			}

		}
		else if (con == '\0')
		{
			fa->~FileAction();
			return -1;
		}
		else
		{
			output(16, "/");
			code = 16;
			retract();
		}
	}
	else
	{
		error("Syntax Error!!");
	}
	return code;

}

void LexicalAnalysis::analysis()
{
	int id = get_next_word();
	if (id < 0)
	{
		return;
	}

	while (id > 0)
	{
		id = get_next_word();

	}

}

void LexicalAnalysis::output(int id, const char* entry)
{
	std::string info;
	char temp[10];

	info.append("(  ");
	sprintf(temp, "%d", id);
	info.append(temp);
	info.append(" ,   ");
	info.append(entry);
	info.append("  )\n");

	//print_info(info, 0);

}

void LexicalAnalysis::concat()
{
	str_token.push_back(ch);
}

int LexicalAnalysis::get_bc(char &ch)
{
	int con;
	while (ch == ' ')
	{
		con = get_char(ch);
		if (con == 0)
		{
			return 0;
		}
	}
	return con;
}

char LexicalAnalysis::get_char(char &ch)
{
	ch = fa->get_char();
	while (ch == '\t' || ch == '\n')
	{
		ch = fa->get_char();//delete the newline
	}
	return ch;
}

std::list<std::string>& LexicalAnalysis::get_const_table()
{
	return const_table;
}

std::list<std::string>& LexicalAnalysis::get_label_table()
{
	return label_table;

}

int LexicalAnalysis::insert_const()
{
	const_table.push_back(str_token);
	return const_table.size() - 1;
}

int LexicalAnalysis::insert_id()
{
	label_table.push_back(str_token);
	return label_table.size() - 1;
}

bool LexicalAnalysis::is_digit(char ch)
{
	if (ch >= '0' && ch <= '9')
	{
		return true;
	}

	return false;
}

bool LexicalAnalysis::is_letter(char ch)
{
	if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
	{
		return true;
	}

	return false;
}

int LexicalAnalysis::reserve()
{
	for (unsigned int i = 0; i < keywords.size(); ++i)
	{
		if (keywords[i] == str_token)
		{
			return i + 1;
		}
	}

	return 0;
}

void LexicalAnalysis::retract()
{

	fa->retract();
	ch = ' ';

}

void LexicalAnalysis::error(const std::string &info)
{

	std::string s;
	char temp[20];
	s.append("line: ");
	memset(temp, '\0', sizeof(temp));
	sprintf(temp, "%d", fa->get_line());
	s.append(temp);
	s.append(" column: ");
	memset(temp, '\0', sizeof(temp));
	sprintf(temp, "%d", fa->get_colume());
	s.append(temp);
	s.append(" ");
	s.append(info);
	s.append("\n");

	print_info(s, 1);

}

void LexicalAnalysis::print_info(const std::string &s, int mode)
{
	switch (mode)
	{
		case 0:
			std::cout << s;
			break;
		case 1:
			std::cerr << s;
			break;
		default:
			std::cerr << "LexicalAnalysis::print_info : unknown mode!!\n";
			break;

	}

}
