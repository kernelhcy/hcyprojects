#include "breakout.h"
#include <QApplication>
#include <QTextCodec>
#include "config.h"

class Config;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
	
	QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF8"));
    
    //Config *config = Config::getInstance();
    
    Breakout *bo = new Breakout();;
    bo -> show();

	/* 销毁config类,只在此销毁一次！！ */
	//delete config;	
	
    return app.exec();
}
