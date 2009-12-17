#ifndef CONFIG_H
#define CONFIG_H
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QtGui>
#include "env.h"

class Config : public QObject
{

public:
    ~Config();
    static Config * getInstance();
    int writeToFile();
    int readFromFile();
    QString proxyHost;
    int proxyPort;
    bool useProxy;

    //显示模式
    ENV1G::viewMode mode;
private:
    Config();
    static Config *_instance;
    QString configFileName;
    void writeComments(QTextStream &out);
    //void readComments(QTextStream &in);
};

#endif // CONFIG_H
