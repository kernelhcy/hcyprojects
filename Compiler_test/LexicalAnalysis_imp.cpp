
#include <iostream>

#include "LexicalAnalysis.h"

LexicalAnalysis::LexicalAnalysis(FileAction& fa_p)
: ch(' '), buffer_size(15), index(0), fa(fa_p), len(0)
{
    //std::cout << "The Lexical Analysis" << std::endl;

    //allocate a buffer
    buffer = new char[buffer_size];

    str_token.clear();

    //prepare the key words table
    /*
     *  The index of the key word add one in the vector is the id of this key word;
     *  For example, 'program''s id is 1.The id of 'then' is 7.
     *                //key word    //Id
     */
    keywords.push_back("program"); //1
    keywords.push_back("begin"); //2
    keywords.push_back("end"); //3
    keywords.push_back("var"); //4
    keywords.push_back("integer"); //5
    keywords.push_back("if"); //6
    keywords.push_back("then"); //7
    keywords.push_back("else"); //8
    keywords.push_back("do"); //9
    keywords.push_back("while"); //10
    
    //open the source file
    fa.open_file();

    //fill the buffer
    len = fill_buffer();
}

LexicalAnalysis::~LexicalAnalysis()
{
    fa.~FileAction();
    delete[] buffer;
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
        fa.close_file();
        return -1;
    };

    if (get_bc(ch) == 0)
    {
        fa.close_file();
        return -1;
    };

    if (is_letter(ch))
    {
        //Get a word!
        while (is_digit(ch) || is_letter(ch))
        {
            concat();
            con = get_char(ch);

            if (con == 0)
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
            output(12, str_token.data());
            code = 12;
        } else
        {
            output(code, str_token.data());
        }

        /*
         *  The buffer is empty which means that the source file has no entry.
         *  Job is done!
         *  Exit.
         */
        if (con == 0)
        {
            fa.close_file();
            return -1;
        }

    } else if (is_digit(ch))
    {
        //Got a constant!
        while (is_digit(ch))
        {
            concat();
            con = get_char(ch);

            if (con == 0)
            {
                break;
            }
        }
        retract();
        value = insert_const();
        output(11, str_token.data());
        code = 11;
        //Done
        if (con == 0)
        {
            fa.close_file();
            exit(0);
        }
    } else if (ch == '=')
    {
        output(19, "=");
        code = 19;
    } else if (ch == '+')
    {
        output(13, "+");
        code = 13;
    } else if (ch == '-')
    {
        output(14, "-");
        code = 14;
    } else if (ch == '*')
    {
        output(15, "*");
        code = 15;
    } else if (ch == '/')
    {
        output(16, "/");
        code = 16;
    } else if (ch == '(')
    {
        output(17, "(");
        code = 17;
    } else if (ch == ')')
    {
        output(18, ")");
        code = 18;
    } else if (ch == '<')
    {
        output(21, "<");
        code = 21;
    } else if (ch == '>')
    {
        output(20, ">");
        code = 20;
    } else if (ch == ';')
    {
        output(22, ";");
        code = 22;
    } else if (ch == ',')
    {
        output(25, ",");
        code = 25;
    } else if (ch == ':')
    {
        con == get_char(ch);

        if (ch == '=')
        {
            output(24, ":=");
            code = 24;
        } else
        {
            retract();
            output(23, ":");
            code = 23;
        }

        //Done
        if (con == 0)
        {
            fa.close_file();
            return -1;
        }
    } else
    {
        std::cerr << "Sytax Error!!\n";

        fa.close_file();
        return -1;
    }
    return code;


}

void LexicalAnalysis::analysis()
{
    int id = get_next_word();

    while (id > 0)
    {
        id = get_next_word();

    }


}

void LexicalAnalysis::output(int id, const char* entry)
{
    //fa.get_ofstrem() << "(  " << id << " ,   " << entry << "  )" << std::endl;
    //std::cout << "(  " << id << " ,   " << entry << "  )" << std::endl;

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

char* LexicalAnalysis::get_buffer()
{
    return buffer;
}

int LexicalAnalysis::get_buffer_size()
{
    return buffer_size;
}

int LexicalAnalysis::get_char(char &ch)
{
    //The buffer is empty
    if (index >= len)
    {
        //Fill the buffer
        len = fill_buffer();

        //NO char!
        //The job has done!!
        //Bye!!~
        if (len == 0)
        {
            return 0;
        }

        index = 0;
        ch = buffer[0];
    } else
    {
        ch = buffer[index];
    }

    ++index;
    return 1;
}

int LexicalAnalysis::fill_buffer()
{
    return fa.fill_buffer(buffer, buffer_size);
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
    for (int i = 0; i < keywords.size(); ++i)
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
    --index;
    if (index < 0)
    {
        index = 0;

    }

    ch = ' ';

}

int LexicalAnalysis::set_buffer_size(int size)
{
    char *temp = NULL;
    temp = new char[size];
    if (temp != NULL)
    {
        delete[] buffer;
        buffer = temp;
        temp = NULL;
    } else
    {
        std::cerr << "Can not resize the buffer!\n";
        exit(1);
    }
}

void LexicalAnalysis::print_buffer()
{
    while(index < buffer_size)
    {
        std::cout<<buffer[index];
        ++index;
    }
    //more 
    fill_buffer();
    std::cout<<buffer;
    std::cout<<"\n";
}

