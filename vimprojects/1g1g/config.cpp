#include "config.h"

Config* Config::_instance = 0;

Config* Config::getInstance()
{
    if (_instance == 0)
    {
        _instance = new Config();
    }

    return _instance;
}

Config::Config()
{
    configFileName = ".linux1g1g.conf";

    proxyHost = "202.117.21.117";
    proxyPort = 3128;
    useProxy = true;
    mode = ENV1G::V_NORMAL;
    readFromFile();
}

Config::~Config()
{
    writeToFile();
}

int Config::readFromFile()
{

    QFile file(configFileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        //printf("Open file failed.\n");
        writeToFile();
        return -1;
    }
    QTextStream in(&file);
    QString info;
    QStringList sl;

    in >> info;
    while (info.at(0) == '#')
    {
        in >> info;
    }
    sl = info.split('=');
    if (sl[1] == "false")
    {
        useProxy = false;
    }
    else
    {
        useProxy = true;
        /*host*/
        in >> info;
        while (info.at(0) == '#')
        {
            in >> info;
        }
        sl = info.split('=');
        proxyHost = sl[1];

        /*port*/
        in >> info;
        while (info.at(0) == '#')
        {
            in >> info;
        }
        sl = info.split('=');
        proxyPort = sl[1].toInt();
    }

    /*view mode*/
    in >> info;
    while (info.at(0) == '#')
    {
        in >> info;
    }
    sl = info.split('=');
    int m = sl[1].toInt();
    switch(m)
    {
    case 0:
        mode = ENV1G::V_NORMAL;
        break;
    case 1:
        mode = ENV1G::V_SIMPLE;
        break;
    case 2:
        mode = ENV1G::V_LISTEN;
        break;
    default:
        mode = ENV1G::V_NORMAL;
        break;
    }

    file.close();

    return 0;
}

int Config::writeToFile()
{

    QFile file(configFileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        //printf("Open file failed.\n");
        return -1;
    }

    QTextStream out(&file);
    writeComments(out);
    if (useProxy)
    {
        out << tr("#设置是否使用代理。\n");
        out << tr("#设置为true或者false\n");
        out << "useproxy=true\n";
        out << tr("#代理服务器名或者IP地址\n");
        out << "proxy_host=" << proxyHost << "\n";
        out << tr("#代理服务器端口\n");
        out << "proxy_port=" << proxyPort << "\n";
    }
    else
    {
        out << tr("#设置是否使用代理。\n");
        out << tr("#设置为true或者false\n");
        out << "useproxy=false\n";
        out << tr("#代理服务器名或者IP地址\n");
        out << "#proxy_host=" << proxyHost << "\n";
        out << tr("#代理服务器端口\n");
        out << "#proxy_port=" << proxyPort << "\n";
    }

    out << tr("#显示模式\n");
    out << "view_mode=" << mode << "\n";
    out.flush();

    file.close();

    return 0;
}
void Config::writeComments(QTextStream &out)
{
    out << tr("#**********亦歌客户端配置文件**********\n");
    out << tr("#!!!!!!!!!!请勿手动修改!!!!!!!!!!!!!!!\n");
    out << tr("#\n");
}
