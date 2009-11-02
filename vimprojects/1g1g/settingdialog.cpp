#include "settingdialog.h"

SettingDialog::SettingDialog(QWidget *_parent)
    :QDialog(_parent)
{
    parent = (MainWindow *)_parent;
    config = Config::getInstance();

    QVBoxLayout *vbox = new QVBoxLayout(this);
    QHBoxLayout *hbox = new QHBoxLayout();

    useProxy = new QCheckBox(tr("使用代理服务器"), this);
    vbox -> addWidget(useProxy);
    /* 这句必须在connect函数之前，否则会报端错误！ */
    if (config -> useProxy)
    {
        useProxy -> setCheckState(Qt::Checked);
    }
    else
    {
        useProxy -> setCheckState(Qt::Unchecked);
    }
    connect(useProxy, SIGNAL(stateChanged(int)), this, SLOT(isChecked(int)));

    QLabel *host_label = new QLabel(tr("地址："));
    QLabel *port_label = new QLabel(tr("端口："));

    host_edit = new QLineEdit(config->proxyHost, this);
    port_edit = new QLineEdit(QString("%1").arg(config->proxyPort), this);

    hbox -> addWidget(host_label);
    hbox -> addWidget(host_edit);

    QHBoxLayout *hbox1 = new QHBoxLayout();
    hbox1 -> addWidget(port_label);
    hbox1 -> addWidget(port_edit);

    vbox -> addLayout(hbox);
    vbox -> addLayout(hbox1);
    if (config -> useProxy)
    {
        host_edit -> setReadOnly(false);
        port_edit -> setReadOnly(false);
    }
    else
    {
        host_edit -> setReadOnly(true);
        port_edit -> setReadOnly(true);
    }

//    QLabel *warnning =new QLabel(tr(" 设置在重启后生效！！"), this);
//    QFont font(tr("宋体"));
//    font.setBold(true);
//    font.setItalic(true);
//    font.setPointSize(15);
//    warnning -> setFont(font);
//    vbox -> addWidget(warnning);

    okBtn = new QPushButton(tr("确定"), this);
    cancelBtn = new QPushButton(tr("取消"), this);

    connect(okBtn, SIGNAL(released()), this, SLOT(ok()));
    connect(cancelBtn, SIGNAL(released()), this, SLOT(cancel()));

    QHBoxLayout *hbox2 = new QHBoxLayout();
    hbox2 -> addWidget(okBtn);
    hbox2 -> addWidget(cancelBtn);
    vbox -> addLayout(hbox2);

    //resize(size());
    setWindowTitle(tr("设置代理"));
    show();

    /* 设置窗口大小不可变 */
    setMaximumSize(size());
    setMinimumSize(size());
}

void SettingDialog::ok()
{
    config->proxyHost = host_edit->text();
    config->proxyPort = (port_edit->text()).toInt();
    config->writeToFile();

    parent -> reStartPlayer();

    close();

}
void SettingDialog::cancel()
{
    close();
}

void SettingDialog::isChecked(int state)
{
    if (state == Qt::Checked)
    {
        host_edit -> setReadOnly(false);
        port_edit -> setReadOnly(false);
        config -> useProxy = true;
    }
    else if (state == Qt::Unchecked)
    {
        host_edit -> setReadOnly(true);
        port_edit -> setReadOnly(true);
        config -> useProxy = false;
    }
}
