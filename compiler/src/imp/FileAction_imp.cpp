#include "../headers/FileActions.h"

FileAction * FileAction::_instance = NULL;

FileAction::FileAction()
{
	in_path = "in";
	out_path = "out";

	init();

}

FileAction::FileAction(const std::string& f_in_path, const std::string& f_out_path) :
	in_path(f_in_path), out_path(f_out_path)
{
	//set_path(f_path);

	init();
}

void FileAction::init(void)
{
	//std::cout << "init..." << std::endl;
	open_file();
	this -> line = 0;
	this -> index = 0;
	this -> buffer_size = 500;
	this -> buffer_len = -1; //set to -1, convenience to the first time to fill the buffer.
	buffer = new char[buffer_size];

}

FileAction::~FileAction(void)
{
	delete[] buffer;
	this->close_file();

}

FileAction* FileAction::get_instance()
{
	if (_instance == NULL)
	{
		_instance = new FileAction();
	}

	return _instance;
}

FileAction* FileAction::get_instance(const std::string& in_path, const std::string& out_path)
{
	if (_instance == NULL)
	{
		_instance = new FileAction(in_path, out_path);
	}

	return _instance;
}

const std::string& FileAction::get_in_path(void)
{
	return in_path;
}

const std::string& FileAction::get_out_path(void)
{
	return out_path;
}

int FileAction::open_file(void)
{
	//open the input source file
	ifs.open(in_path.c_str(), std::ios::in | std::ios::out);

	//open the output file
	ofs.open(out_path.c_str(), std::ios::out);

	return 1;
}

int FileAction::close_file(void)
{
	if (ifs.is_open())
	{
		ifs.close();
	}
	//std::cout << "closing input source file...\n";

	if (ofs.is_open())
	{
		ofs.close();
	}
	//std::cout << "closing output file...\n";
	return 0;
}

int FileAction::fill_buffer(void)
{

	memset(buffer, '\0', buffer_size);

	if (!ifs.eof())
	{
		/*
		 * 回车换行符放在下一行的开始位置！！！
		 */
		ifs.getline(buffer, 500);
		//std::cout << buffer << std::endl;
	}
	else
	{
		return -1;
	}

	return strlen(buffer);
}

int FileAction::next_line(void)
{

	//read next line which is not bank line
	do
	{
		buffer_len = fill_buffer();
		if (buffer_len == -1)//The end of the file
		{
			return -1;
		}
		++line;
		index = 0;
	}while (buffer_len <= 1 && (buffer[0] == '\0' || buffer[0] == '\t' || buffer[0] == '\n'));
	return 0;
}

char FileAction::get_char(void)
{
	char temp = ' ';

	if (index >= buffer_size)//the buffer is overflowed!
	{
		std::cerr << "FileAction ERROR : Buffer overflow!!" << std::endl;
		close_file();
		exit(1);
	}

	int state = 0;
	if(index > buffer_len || buffer[index] == '\0')
	{
		state = next_line();
	}

	//end of file
	if(state == -1)
	{
		return '\0';
	}

	temp = buffer[index];
	++index;

	return temp;
}

int FileAction::get_colume(void)
{
	return this->index;
}

int FileAction::get_line(void)
{
	return this->line;
}

void FileAction::retract(void)
{
	--index;
	if (index < 0)
	{
		std::cout << "FileAction::retract  index < 0\n";
	}
}
