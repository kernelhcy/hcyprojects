#ifndef CONFIG_H
#define CONFIG_H
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QtGui>

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
private:
    Config();
    static Config *_instance;
    QString configFileName;
    void writeComments(QTextStream &out);
    //void readComments(QTextStream &in);
};

#endif // CONFIG_H
