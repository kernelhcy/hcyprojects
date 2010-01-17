/***************************************************************************
 *   Copyright (C) 2009 by silwings,ericyosho   *
 *   silwings@gmail.com, ericyosho@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QtGui/QApplication>
#include <QtNetwork>
#include <cstdlib>
#include <QTextStream>
#include <QTextCodec>
#include <QWindowsVistaStyle>
#include "mainwindow.h"
#include "controlthread.h"

int main(int argc, char *argv[])
{  
    system("xbindkeys");
    QTextCodec::setCodecForTr(QTextCodec::codecForName("utf-8"));
    QApplication a(argc, argv);
    //QApplication::setStyle(new QWindowsVistaStyle());
    /*
    QString proxy_s = "http://202.117.21.117:3128/";

    if (argc == 2)
    {
        proxy_s = argv[1];
    }

    QString host;
    int port;
    QRegExp regex("(http://)?(.*):(\\d*)/?");
    int pos = regex.indexIn(proxy_s);
    if(pos > -1){
        host = regex.cap(2);
        port = regex.cap(3).toInt();
    }
    */

    ControlThread ct;
    MainWindow w((QWidget*)0,&ct);
    QObject::connect((QObject*)&ct, SIGNAL(recvcommand(QString, QString))
                     , (QObject*)&w, SLOT(processCommand(QString, QString))
                     , Qt::QueuedConnection);
    w.show();
    ct.start();    
    return a.exec();;
}
