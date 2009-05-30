
#include "SyntaxAnalysis.h"
#include <string>

SyntaxAnalysis::SyntaxAnalysis(LexicalAnalysis & _la)
: la(_la)
{
    //initialize
    info = 0;

    string_id[0] = "p";
    string_id[1] = "b";
    string_id[2] = "n";
    string_id[3] = "v";
    string_id[4] = "g";
    string_id[5] = "f";
    string_id[6] = "t";
    string_id[7] = "e";
    string_id[8] = "d";
    string_id[9] = "w";
    string_id[10] = "c";
    string_id[11] = "i";
    string_id[12] = "+";
    string_id[13] = "-";
    string_id[14] = "*";
    string_id[15] = "/";
    string_id[16] = "(";
    string_id[17] = ")";
    string_id[18] = "+";
    string_id[19] = ">";
    string_id[20] = "<";
    string_id[21] = ";";
    string_id[22] = ":";
    string_id[23] = ":=";
    string_id[24] = ",";

    //
    table1[0] = "B";
    table1[1] = "A";
    table1[2] = "fitAI";
    table1[3] = "widA";
    table1[4] = "i:=E;";
    table1[5] = "TG";
    table1[6] = "FH";
    table1[7] = "+TG";
    table1[8] = "-TG";
    table1[9] = "i";
    table1[10] = "(E)";
    table1[11] = "*FH";
    table1[12] = "/FH";
    table1[13] = "eA";
    table1[14] = "";

    for (int i = 0; i < SIZE; ++i)
    {
        for (int j = 0; j < SIZE; ++j)
        {
            table2[i][j] = -1;
        }
    }
    //initialize the predicrion analysis table
    table2['S' - 'A'][6] = 0;
    table2['S' - 'A'][10] = 0;
    table2['S' - 'A'][12] = 1;

    table2['A' - 'A'][12] = 4;

    table2['B' - 'A'][6] = 2;
    table2['B' - 'A'][10] = 3;

    table2['E' - 'A'][12] = 5;
    table2['E' - 'A'][17] = 5;

    table2['T' - 'A'][12] = 6;
    table2['T' - 'A'][17] = 6;

    table2['G' - 'A'][13] = 7;
    table2['G' - 'A'][14] = 8;
    table2['G' - 'A'][18] = 14;
    table2['G' - 'A'][24] = 14;
    table2['G' - 'A'][12] = 14;
    table2['G' - 'A'][22] = 14;
    table2['G' - 'A'][15] = 14;
    table2['G' - 'A'][16] = 14;

    table2['F' - 'A'][12] = 9;
    table2['F' - 'A'][17] = 10;

    table2['H' - 'A'][12] = 14;
    table2['H' - 'A'][13] = 14;
    table2['H' - 'A'][14] = 14;
    table2['H' - 'A'][15] = 11;
    table2['H' - 'A'][16] = 12;
    table2['H' - 'A'][18] = 14;
    table2['H' - 'A'][22] = 14;

    table2['I' - 'A'][8] = 13;

}

SyntaxAnalysis::~SyntaxAnalysis()
{

    la.~LexicalAnalysis();
}

void SyntaxAnalysis::analysis()
{
    //get the next word's id
    int word_id = la.get_next_word();

    //put the begin label into the stack
    s.push('S');

    std::string temp;
    int id;
    char temp_ch;
    while (word_id > 0)//loop until over or error
    {
        //print the content of the stack
        //print_stack();



        //get the char on the top of the stack
        temp_ch = s.top();
        s.pop(); //pop the top char

        //a temp string
        temp.clear();
        temp += temp_ch;

        //std::cout << temp << std::endl;

        //The := has two characters.
        if (temp_ch == ':' && s.top() == '=')
        {
            s.pop();

            temp += "=";
        }

        //This char is not a final label
        if (temp_ch >= 'A' && temp_ch <= 'Z')//not final label
        {
            int temp_id = table2[temp_ch - 'A'][word_id];

            //the if then else bug
            if (temp_id < 0 && temp_ch == 'I' && s.empty())
            {
                //print the information.
                print_info();

                //clear everything
                info = 0;
                
                //word_id = la.get_next_word();
                //begin the next sentence
                s.push('S');
                continue;
            }

            //Got an ERROR!!
            if (temp_id < 0)
            {
                std::cout << "语法错误！！位置：";
                print_error_position();
                exit(1);
            }

            //Just print the infomation of the sentence.
            //Nothing to do with the analysis...
            if (temp_ch == 'S')
            {
                push_all(table1[temp_id], true);
            } else
            {
                push_all(table1[temp_id], false);
            }

        } else// is final label
        {
            id = get_id(temp);

            if (word_id == id)//
            {
                if (s.empty())//get a sentence.
                {
                    //print the information.
                    print_info();

                    //clear everything
                    info = 0;
                    while (s.size() > 0)
                    {
                        s.pop();
                    }
                    //begin the next sentence
                    word_id = la.get_next_word();
                    s.push('S');

                } else
                {
                    word_id = la.get_next_word();

                    //Over.
                    if (word_id <= 0)
                    {
                        //                        //the if then else bug
                        //                        print_stack();
                        //                        print_info();
                        //                        info = 0;
                        //
                        //                        while (s.size() > 0)
                        //                        {
                        //                            s.pop();
                        //                        }
                        break;
                    }

                    if (word_id == 11)//constant is dealed with label
                    {
                        word_id = 12;
                    }
                }

            } else
            {
                std::cout << "语法错误！！位置: ";
                print_error_position();
                exit(1);
            }
        }


    }
}

bool SyntaxAnalysis::is_finalword(int id)
{
    if (id > 0 && id < 15)
    {
        return true;
    }

    return false;
}

void SyntaxAnalysis::push_all(const std::string& str, bool flag)
{

    int len = str.length();

    //rember the information about this sentence
    if (len > 0 && str[1] == ':')
    {
        if (info > 0)
        {
            info += 8;
        } else
        {
            info = 8;
        }

    }
    if (len > 0 && str[0] == 'f')
    {
        info = 1;
    }
    if (len > 0 && str[0] == 'w')
    {
        info = 4;
    }
    if (len > 0 && str[0] == 'e')
    {
        info = 2;
    }

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
            return i + 1;
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

void SyntaxAnalysis::print_info()
{
    if (info == 1)
    {
        std::cout << "if-then分支语句。\n";
    } else if (info == 2)
    {
        std::cout << "if-then-else分支语句。\n";
    } else if (info == 4)
    {
        std::cout << "while-do循环语句。\n";
    } else if (info == 8)
    {
        std::cout << "赋值语句。\n";
    } else if (info == 9)
    {
        std::cout << "if-then分支语句，嵌套赋值语句。\n";
    } else if (info == 10)
    {
        std::cout << "if-then-else分支语句，嵌套赋值语句。\n";
    } else if (info == 12)
    {
        std::cout << "while-do循环语句，嵌套赋值语句。\n";
    }
}

void SyntaxAnalysis::print_error_position()
{
    //char * buffer = la.get_buffer();
//    int len = la.get_buffer_size();
//    int begin = 0;
//    while (buffer[begin] != ';')
//    {
//        std::cout << buffer[begin];
//        ++begin;
//        if (begin >= len)
//        {
//            la.fill_buffer();
//        }
//    }
    //std::cout <<buffer<< "\n";
    la.print_buffer();
}

