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

#ifndef CONTROLTHREAD_H
#define CONTROLTHREAD_H

#include <QThread>
#include <QObject>
#include <QFile>
#include <QDebug>
#include <linux/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

class ControlThread : public QThread
{
    Q_OBJECT
public:
    ControlThread();
    void run();
    signals:
        void recvcommand(QString, QString);
protected:    
    bool isValidCommand(char*);
};

#endif // CONTROLTHREAD_H
