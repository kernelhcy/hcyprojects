#ifndef _CONFIG_H
#define _CONFIG_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QStringList>
/*
 * 配置信息类
 * 保存配置信息，同时负责读写配置文件
 */

class Config
{
	public :
		static Config* getInstance();
		~Config();
		
		/* 砖块的行数和列数 */
		int col, line;
		
		/* 游戏的速度 */
		int speed;
		
		/*  
		 * 游戏的模式
		 * 1 : 随机模式
		 * 0 : 正常模式
		 */
		int mode;
		/* 读写配置文件 */
		/*
		 * 在配置文件中，各个配置项的保存模式如下：
		 * 
		 *	 line=5
		 *	 col=6
		 *	 speed=10
		 *	 ...
		 *
		 * 配置文件的名称为 : breakout.conf
		 *
		 * 成功返回0，失败返回-1
		 *
		 */
		int writeToFile();
		int readFromFile();

	private :
		/* 单例模式 */
		static Config* _instance;
		Config();

		QString configFileName;

};

#endif
