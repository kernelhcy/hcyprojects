#include "config.h"

Config* Config::_instance = 0;

Config* Config::getInstance()
{
	if (_instance == 0)
	{
		_instance = new Config();
	}

	return _instance;
}

Config::Config()
{
	configFileName = "breakout.conf";
	
	/* 默认设置 */
	line = 5;
	col = 6;
	speed = 10;
	mode = 0;

	readFromFile();
}

Config::~Config()
{
	writeToFile();
}

int Config::readFromFile()
{
	//printf("read configure file.\n");

	QFile file(configFileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		//printf("Open file failed.\n");
		writeToFile();
		return -1;
	}
    QTextStream in(&file);
	QString info;
	QStringList sl;
	
	/*line*/
	in >> info;
	sl = info.split('=');
	line = sl[1].toInt();

	/*col*/
	in >> info;
	sl = info.split('=');
	col = sl[1].toInt();

	/*speed*/
	in >> info;
	sl = info.split('=');
	speed = sl[1].toInt();

	/*mode*/
	in >> info;
	sl = info.split('=');
	mode = sl[1].toInt();

	//printf("line %d\ncol %d\nspeed %d\nmode %d\n"
	//		,line, col, speed, mode);

	file.close();
	
	return 0;
}

int Config::writeToFile()
{
	//printf("write configure file.\n");
	
	QFile file(configFileName);
	if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
	{
		//printf("Open file failed.\n");
		return -1;
	}

	QTextStream out(&file);

	out << "line=" << line << "\n";
	out << "col=" << col << "\n";
	out << "speed=" << speed << "\n";
	out << "mode=" << mode << "\n";

	out.flush();

	file.close();
	
	return 0;
}
