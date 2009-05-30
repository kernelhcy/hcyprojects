#include "FileActions.h"

FileAction::FileAction(std::ifstream& ifs_p, std::ofstream& ofs_p) : ifs(ifs_p), ofs(ofs_p)
{
    in_path = "in";
    out_path = "out";

}

FileAction::FileAction(const std::string& f_in_path, const std::string& f_out_path
        , std::ifstream& ifs_p, std::ofstream& ofs_p)
: in_path(f_in_path), out_path(f_out_path), ifs(ifs_p), ofs(ofs_p)
{
    //set_path(f_path);

}

FileAction::~FileAction()
{
    //delete[] path;
    this->close_file();

}

const std::string& FileAction::get_in_path()
{
    return in_path;
}

void FileAction::set_in_path(const std::string& path)
{
    this->in_path = path;
}

const std::string& FileAction::get_out_path()
{
    return out_path;
}

void FileAction::set_out_path(const std::string& path)
{
    this->out_path = path;
}

int FileAction::open_file()
{
    //open the input source file
    ifs.open(in_path.data(), std::ios::in | std::ios::out);



    //openning input file failed.
    if (ifs == NULL)
    {
        std::cerr << "Open Source File Error!" << std::endl;
        return 0;
    }

    //open the output file

    ofs.open(out_path.data(), std::ios::out);

    if (ofs == NULL)
    {
        std::cerr << "Open Output File Error!" << std::endl;
        return 0;
    }


    return 1;
}

int FileAction::close_file()
{
    ifs.close();
    //std::cout<<"closing input source file...\n";
    ofs.close();
    //std::cout<<"closing output file...\n";
    return 1;
}

int FileAction::fill_buffer(char* buffer, int size)
{
    //std::cout<<"fill buffer \n";

    //clear up the buffer
    memset(buffer, '\0', size);

    char temp = ' ';
    int index = 0;
    if (ifs == NULL)
    {
        return 0;
    }

    while (!ifs.eof() && size > 0)
    {
        //get a character
        ifs.get(temp);

        //delete the enter charater
        if (temp == '\n')
        {
            continue;
        }


        if (ifs.eof())//end of the file
        {
            break;
        }

        buffer[index] = temp;
        ++index;


        //delete the noused black space
        if (temp == ' ' || temp == '\n')
        {
            while (temp == ' ' || temp == '\n')
            {
                ifs.get(temp);
            }

            //put the first character while is not ' ' into the buffer
            buffer[index] = temp;
            ++index;

            //the capacity of the buffer 
            --size;
            if (size <= 0)
            {
                break;
            }
        }

        --size;
    }

    //std::cout<<buffer<<std::endl;

    return index;
}

char FileAction::get_char()
{
    char temp = ' ';
    ifs.get(temp);
    return temp;
}

std::ofstream& FileAction::get_ofstrem()
{
    return this->ofs;
}
