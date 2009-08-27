#include "../headers/SyntaxAnalysis.h"
#include <string>

//initialize the only instance
SyntaxAnalysis* SyntaxAnalysis::_instance = NULL;

SyntaxAnalysis* SyntaxAnalysis::get_instance(const std::string& in_path,
		const std::string& out_path)
{
	if (_instance == NULL)
	{
		_instance = new SyntaxAnalysis(in_path, out_path);
	}

	return _instance;
}
SyntaxAnalysis* SyntaxAnalysis::get_instance()
{
	return _instance;
}
SyntaxAnalysis::SyntaxAnalysis(const std::string& in_path, const std::string& out_path)
{
	//initialize

	scope = 0;
	//get the only instance of LexicalAnalysis class
	la = LexicalAnalysis::get_instance(in_path, out_path);
	//get the only instance of FileAction class
	fa = FileAction::get_instance(in_path, out_path);

	cm = CreateMidcode::get_instance();

	string_id[0] = "";//no use
	string_id[1] = "m";//main
	string_id[2] = "{";
	string_id[3] = "}";
	string_id[4] = "b";//bool
	string_id[5] = "n";//int
	string_id[6] = "f";//if
	string_id[7] = "r";//return
	string_id[8] = "e";//else
	string_id[9] = "d";//do : no use
	string_id[10] = "w";//while
	string_id[11] = "s";//false
	string_id[12] = "t";//true
	string_id[13] = "+";
	string_id[14] = "-";
	string_id[15] = "*";
	string_id[16] = "/";
	string_id[17] = "(";
	string_id[18] = ")";
	string_id[19] = "=";
	string_id[20] = ">";
	string_id[21] = "<";
	string_id[22] = ";";
	string_id[23] = "==";
	string_id[24] = "!=";
	string_id[25] = ",";
	string_id[26] = "&";
	string_id[27] = "|";
	string_id[28] = "!";
	string_id[29] = "/*";
	string_id[30] = "*/";
	string_id[31] = "//";
	string_id[32] = "c";//constant
	string_id[33] = "i";//label

	//all strings that are in the generation formula
	table1[0] = "nm(){XSArF;}";//int main(){A return i;}
	table1[1] = "BA";
	table1[2] = "CA";
	table1[3] = "i=E;";
	table1[4] = "GI";
	table1[5] = "J";
	table1[6] = "TO";
	table1[7] = "+TO";
	table1[8] = "-TO";
	table1[9] = "PH";
	table1[10] = "*PH";
	table1[11] = "/FQ";
	table1[12] = "(E)";
	table1[13] = "i";
	table1[14] = "f(E)R{A}K";//if(K){A}I
	table1[15] = "wR(E)R{A}";//while(K){A}
	table1[16] = "UeR{A}";//else A
	table1[17] = "niXMN";//int i N
	table1[18] = "biXMN";//bool i N
	table1[19] = ",iXNN";
	table1[20] = "!GI";
	table1[21] = "(I)J";//no use
	table1[22] = "iJ";// no use
	table1[23] = "t";//true
	table1[24] = "s";//false
	table1[25] = "&RGI";
	table1[26] = "|RGI";
	table1[27] = "";//nothing
	table1[28] = "LA";
	table1[29] = "M;";
	table1[30] = "N";
	table1[31] = "FJ";
	table1[32] = "O";
	table1[33] = "c";//constants
	table1[34] = "FQ";
	table1[35] = "P";
	table1[36] = "Q";
	table1[37] = "R";
	table1[38] = "S";
	table1[39] = ">GI";
	table1[40] = "<GI";
	table1[41] = "==GI";
	table1[42] = "!=GI";

	//no use
	table1[39] = "c";

	for (int i = 0; i < SIZE; ++i)
	{
		for (int j = 0; j < SIZE; ++j)
		{
			table2[i][j] = -1;
		}
	}
	//initialize the predicrion analysis table
	table2['S' - 'A'][5] = 0;

	table2['A' - 'A'][4] = 28;
	table2['A' - 'A'][5] = 28;
	table2['A' - 'A'][6] = 2;
	table2['A' - 'A'][7] = 27;
	table2['A' - 'A'][10] = 2;
	table2['A' - 'A'][33] = 1;
	table2['A' - 'A'][3] = 27;

	table2['B' - 'A'][33] = 3;

	table2['C' - 'A'][6] = 14;
	table2['C' - 'A'][10] = 15;

	table2['E' - 'A'][33] = 4;
	table2['E' - 'A'][32] = 4;
	table2['E' - 'A'][17] = 4;
	table2['E' - 'A'][11] = 4;
	table2['E' - 'A'][12] = 4;

	table2['F' - 'A'][33] = 13;
	table2['F' - 'A'][32] = 33;
	table2['F' - 'A'][17] = 12;
	table2['F' - 'A'][11] = 24;
	table2['F' - 'A'][12] = 23;

	table2['T' - 'A'][33] = 9;
	table2['T' - 'A'][32] = 9;
	table2['T' - 'A'][17] = 9;
	table2['T' - 'A'][11] = 9;
	table2['T' - 'A'][12] = 9;

	table2['P' - 'A'][33] = 34;
	table2['P' - 'A'][32] = 34;
	table2['P' - 'A'][17] = 34;
	table2['P' - 'A'][11] = 34;
	table2['P' - 'A'][12] = 34;

	table2['G' - 'A'][33] = 6;
	table2['G' - 'A'][32] = 6;
	table2['G' - 'A'][17] = 6;
	table2['G' - 'A'][11] = 6;
	table2['G' - 'A'][12] = 6;
	table2['G' - 'A'][28] = 27;

	table2['H' - 'A'][13] = 27;
	table2['H' - 'A'][14] = 27;
	table2['H' - 'A'][15] = 10;
	table2['H' - 'A'][18] = 27;
	table2['H' - 'A'][22] = 27;
	table2['H' - 'A'][26] = 27;
	table2['H' - 'A'][27] = 27;
	table2['H' - 'A'][20] = 27;
	table2['H' - 'A'][21] = 27;
	table2['H' - 'A'][23] = 27;
	table2['H' - 'A'][24] = 27;

	table2['Q' - 'A'][13] = 27;
	table2['Q' - 'A'][14] = 27;
	table2['Q' - 'A'][16] = 11;
	table2['Q' - 'A'][18] = 27;
	table2['Q' - 'A'][22] = 27;
	table2['Q' - 'A'][26] = 27;
	table2['Q' - 'A'][27] = 27;
	table2['Q' - 'A'][20] = 27;
	table2['Q' - 'A'][21] = 27;
	table2['Q' - 'A'][23] = 27;
	table2['Q' - 'A'][24] = 27;
	table2['Q' - 'A'][15] = 27;

	table2['I' - 'A'][26] = 25;
	table2['I' - 'A'][27] = 26;
	table2['I' - 'A'][28] = 20;
	table2['I' - 'A'][18] = 27;
	table2['I' - 'A'][19] = 27;
	table2['I' - 'A'][22] = 27;
	table2['I' - 'A'][33] = 27;
	table2['I' - 'A'][20] = 39;
	table2['I' - 'A'][21] = 40;
	table2['I' - 'A'][23] = 41;
	table2['I' - 'A'][24] = 42;

	table2['J' - 'A'][22] = 27;
	table2['J' - 'A'][18] = 27;
	table2['J' - 'A'][26] = 25;
	table2['J' - 'A'][27] = 26;
	table2['J' - 'A'][20] = 25;
	table2['J' - 'A'][21] = 26;
	table2['J' - 'A'][23] = 25;
	table2['J' - 'A'][24] = 26;

	table2['K' - 'A'][8] = 16;
	table2['K' - 'A'][22] = 27;
	table2['K' - 'A'][26] = 26;
	table2['K' - 'A'][27] = 25;
	table2['K' - 'A'][20] = 26;
	table2['K' - 'A'][21] = 25;
	table2['K' - 'A'][23] = 26;
	table2['K' - 'A'][24] = 25;

	table2['L' - 'A'][5] = 29;
	table2['L' - 'A'][4] = 29;

	table2['M' - 'A'][5] = 17;
	table2['M' - 'A'][4] = 18;

	table2['N' - 'A'][25] = 19;
	table2['N' - 'A'][22] = 27;

	table2['O' - 'A'][33] = 27;
	table2['O' - 'A'][13] = 7;
	table2['O' - 'A'][14] = 8;
	table2['O' - 'A'][18] = 27;
	table2['O' - 'A'][19] = 27;
	table2['O' - 'A'][22] = 27;
	table2['O' - 'A'][26] = 27;
	table2['O' - 'A'][27] = 27;
	table2['O' - 'A'][20] = 27;
	table2['O' - 'A'][21] = 27;
	table2['O' - 'A'][23] = 27;
	table2['O' - 'A'][24] = 27;

	table2['U' - 'A'][8] = 27;

	table2['R' - 'A'][17] = 27;
	table2['R' - 'A'][2] = 27;
	table2['R' - 'A'][11] = 27;
	table2['R' - 'A'][12] = 27;
	table2['R' - 'A'][17] = 27;
	table2['R' - 'A'][32] = 27;
	table2['R' - 'A'][33] = 27;
	table2['R' - 'A'][28] = 27;

}

SyntaxAnalysis::~SyntaxAnalysis()
{

	delete la;
	la = NULL;
}

int SyntaxAnalysis::analysis()
{
	//put the begin label into the stack
	s.push('S');

	std::string temp;
	int id, temp_id;
	//store the left part id of the generation
	std::stack<int> left_part_id;

	char temp_ch = 'S';

	//get the next word's id
	int word_id = la->get_next_word();

	while (word_id > 0)//loop until over or error
	{
		//print the content of the stack
		print_stack();

		//get the char on the top of the stack
		temp_ch = s.top();
		s.pop(); //pop the top char

		//a temp string
		temp.clear();
		temp += temp_ch;

		if (temp_ch == 'X')//create the four tuple
		{
			//std::cout << table1[left_part_id.top()] << " \n";
			//cm -> print_variable_table();
			if (cm -> create(s.top(), left_part_id.top(), scope) > 0)
			{
			}
			//sen.append(la->get_string(word_id));
			s.pop();
			left_part_id.pop();

		}
		else if (temp_ch >= 'A' && temp_ch <= 'Z')//not final label
		{

			temp_id = table2[temp_ch - 'A'][word_id];

			/*
			 * 对于if else 语句，进行特殊处理
			 */
			if (temp_ch == 'K' && table1[temp_id] == "")
			{
				s.push(temp_ch);
				s.push('X');
				left_part_id.push(27);
			}

			//the if then else bug
			if (temp_id < 0 && temp_ch == 'K')
			{
				continue;
			}

			//Got an ERROR!!
			if (temp_id < 0)
			{
				if (word_id == 33 || word_id == 32)
				{
					print_error("多余的变量或常量。");
				}
				else
				{
					print_error("缺少运算符。");
				}

			}

			/*
			 * 向栈中压入产生式的左部，用于确定所要调用的产生四元式的函数。
			 * 同时向栈中压入X，以标记产生四元式的时间点，以防产生混淆。
			 *
			 * 对于说明定义表达式。由于和其他语句的翻译方式不同，给予特殊处理。
			 */
			if (temp_ch != 'M' && temp_ch != 'N' && table1[temp_id] != "")
			{
				s.push(temp_ch);
				s.push('X');
			}
			/*
			 * 如果候选式为空，则不须生成四元式
			 */
			if (table1[temp_id] != "")
			{
				left_part_id.push(temp_id);
			}

			/*
			 * Mquad
			 */
			if (temp_ch == 'R')
			{
				cm -> R(0);
			}
			/*
			 * Mquad
			 */
			if (temp_ch == 'U')
			{
				cm -> U(0);
			}

			//将产生式的右部反向压栈。
			push_all(table1[temp_id]);

		}
		else// is final label
		{
			if (temp_ch == '{')
			{
				++scope;
			}
			else if (temp_ch == '}')
			{
				--scope;
			}

			id = get_id(temp);

			if (word_id == id)//
			{
				if (s.size() == 5 && s.top() == 'A')//get a sentence.
				{
					//cm -> create(sen);
				}

				word_id = la->get_next_word();
				//std::cout << word_id << std::endl;
				//Over.
				if (word_id <= 0)
				{
					break;
				}

				if (word_id == 33)//a label
				{
					cm -> store_variables_ids(la -> get_pos());
				}
				else if (word_id == 32)//a constant
				{
					cm -> store_variables_ids(la -> get_pos() + cm -> VAR_CONS);
					char tmp[10];
					sprintf(tmp, "%d", la -> get_pos());
					cm -> fill(tmp, INT_T, scope, true);
				}
			}
			else
			{
				if (word_id == 33 || word_id == 32)
				{
					print_error("多余的变量或常量。");
				}
				else
				{
					print_error("缺少运算符。");
				}

			}
		}

	}

	//cm -> print_variable_table();
	cm -> print_tuples();
	return 0;
}

bool SyntaxAnalysis::is_finalword(int id)
{
	if (id > 0 && id < 15)
	{
		return true;
	}

	return false;
}

void SyntaxAnalysis::push_all(const std::string& str)
{

	int len = str.length();

	//push them
	for (int i = len - 1; i >= 0; --i)
	{
		s.push(str[i]);
	}
}

int SyntaxAnalysis::get_id(const std::string &str)
{

	for (int i = 0; i < SIZE; ++i)
	{
		//std::cout<<string_id[i]<<str;
		if (string_id[i] == str)
		{
			return i;
		}
	}
	return -1;
}

void SyntaxAnalysis::print_stack()
{
	std::stack<char> temp(s);
	std::cout << "Stack: ";
	int size = temp.size();
	for (int i = 0; i < size; ++i)
	{
		std::cout << temp.top();
		temp.pop();
	}
	std::cout << "\n";
}

void SyntaxAnalysis::print_error(const std::string &info)
{
	std::string s;
	char tmp[10];

	s.append("Syntax Error : ( line ");
	sprintf(tmp, "%d", fa->get_line());
	s.append(tmp);

	s.append(" column ");
	sprintf(tmp, "%d", fa->get_colume());
	s.append(tmp);
	s.append(" ) ");
	s.append(info);

	print_info(s, 0);//error information

	exit(1);

}

void SyntaxAnalysis::print_info(const std::string &s, int mode)
{
	switch (mode)
	{
		case 0:
			std::cerr << s << std::endl;
			break;
		case 1:
			std::cout << s << std::endl;
			break;
		default:
			std::cerr << "print_info : Bad mode!!\n";
			break;
	}
}
