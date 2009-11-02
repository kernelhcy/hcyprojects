#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QCheckBox>
#include "config.h"
#include "mainwindow.h"

class MainWindow;

class SettingDialog : public QDialog
{
    Q_OBJECT

public:
    SettingDialog(QWidget *parent = 0);

private:
    QPushButton *okBtn;
    QPushButton *cancelBtn;
    Config *config;
    QLineEdit *host_edit;
    QLineEdit *port_edit;
    QCheckBox *useProxy;
    MainWindow *parent;
private slots:
    void ok();
    void cancel();
    void isChecked(int);

};

#endif // SETTINGDIALOG_H
