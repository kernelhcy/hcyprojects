

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

#include <qapplication.h> 
#include <qlabel.h> 
#include <qpushbutton.h>
#include <QSlider>

#include <QSpinBox>
#include <QHBoxLayout>
void handler();

int main(int argc, char **argv) 
{ 

	QApplication app(argc, argv); 

//	QLabel *label = new QLabel("<h2><i>Hello</i>,<font color=red> world!</font></h2>", 0 ); 
//	label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter ); 
//	label->setGeometry(10, 10, 200, 80); 
//	label->show(); 

//	QPushButton *button=new QPushButton("Hello,world!", 0);
//	QObject::connect(button,SIGNAL(clicked()),&app,SLOT(handler()));
//	button->resize(100,30);
//	button->show();

	QWidget *window = new QWidget();
	window -> setWindowTitle("Entet Your Age:");

	QSpinBox *spinbox = new QSpinBox();
	QSlider *slider = new QSlider(Qt::Horizontal);
	spinbox -> setRange(0,130);
	slider -> setRange(0,130);
	QObject::connect(spinbox,SIGNAL(valueChanged(int)),slider,SLOT(setValue(int)));
	QObject::connect(slider,SIGNAL(valueChanged(int)),spinbox,SLOT(setValue(int)));
	spinbox -> setValue(35);

	QHBoxLayout *layout = new QHBoxLayout;

	layout->addWidget(spinbox);

	layout->addWidget(slider);

	window->setLayout(layout);

	window->show();


	return app.exec(); 
} 

void handler()
{
	printf("exit...\n");
}
