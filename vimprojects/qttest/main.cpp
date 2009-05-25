

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

int main(int argc, char **argv) 
{ 

//	QApplication app(argc, argv); 

//	QLabel *label = new QLabel( "Hello, world!", 0 ); 

//	label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter ); 

//	label->setGeometry(10, 10, 200, 80); 
	
//	label->show(); 

//	QPushButton hello("Hello word!", 0);
//	hello.resize(100,30);
//	hello.show();

//	int result = app.exec(); 

	QApplication app(argc, argv);

	FindDialog *dialog = new FindDialog();

	dialog -> show();

	return app.exec(); 

} 
