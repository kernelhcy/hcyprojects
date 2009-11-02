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

#include "controlthread.h"
#define FIFO "/tmp/linux1g1g"

ControlThread::ControlThread()
{

}
void ControlThread::run(){
    umask(0);
    mknod(FIFO, S_IFIFO|0666, 0);

    char buf[25];
    while(1){
        FILE* fp = fopen(FIFO,"r");
        if(!fp)
        {
            qDebug()<<"error when open "<<FIFO;
            return;
        }
        fgets(buf, sizeof(buf), fp);        
        buf[strlen(buf)-1] = '\0';        
        if(isValidCommand(buf))
            emit recvcommand(QString(buf));
        fclose(fp);
    }
}

bool ControlThread::isValidCommand(char* buf){
    if(strcmp(buf, "next")==0) return true;
    if(strcmp(buf, "playPause")==0) return true;
    if(strcmp(buf, "volumeOnOff")==0) return true;
    if(strcmp(buf, "volumeUp")==0) return true;
    if(strcmp(buf, "volumeDown")==0) return true;
    return false;
}
