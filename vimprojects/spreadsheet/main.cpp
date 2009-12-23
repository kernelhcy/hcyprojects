

/********************************************************************* 
 *
 * ** $Id: /sample/1/helloworld.cpp   2.3.2   edited 2004-10-12 $ 
 *
 * ** 
 *
 * ** Copyright (C) 2004-2005 OURSELEC AS.    All rights reserved. 
 *
 * ** 
 *
 * ** This file is part of an example program for Qt.    This example 
 *
 * ** program may be used, distributed and modified without limitation. 
 *
 * ** 
 *
 * **********************************************************************
 *
 * ***/ 

#include <QApplication> 
#include "finddialog.h" 
#include "mainwindow.h"
#include <QTextCodec>
int main(int argc, char **argv) 
{ 

	QTextCodec::setCodecForTr(QTextCodec::codecForName("utf-8"));
	QApplication app(argc, argv); 
/*
	QLabel *label = new QLabel( "Hello, world!", 0 ); 

	label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter ); 

	label->setGeometry(10, 10, 200, 80); 
	
	label->show(); 
*/
    QSplashScreen *splash = new QSplashScreen;
    splash->setPixmap(QPixmap(":/images/pics/splash.png"));
    splash->show();
    Qt::Alignment topRight = Qt::AlignRight | Qt::AlignTop;
    splash->showMessage(QObject::tr("Setting up the main window..."), topRight, Qt::white);
    MainWindow mainWin;
    splash->showMessage(QObject::tr("Loading modules..."), topRight, Qt::white);
    //loadModules();
    sleep(2);
    mainWin.show();
    splash->finish(&mainWin);
    delete splash;

	return app.exec();

} 
