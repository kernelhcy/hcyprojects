#ifndef YGMAINWINDOW_H
#define YGMAINWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QIcon>
#include "ygtitlebar.h"

class YGTitleBar;
/**
 * 自定义主窗口。
 * Qt提供的主窗口，标题栏不包括在窗口中。对隐藏操作很麻烦。
 * 自定义一个主窗口。标题栏包含在窗口中。
 */
class YGMainWindow : public QWidget
{
Q_OBJECT
public:
    YGMainWindow(QWidget *parent = 0, Qt::WindowFlags flag =
                                         (Qt::Window |
                                          Qt::FramelessWindowHint |
                                          Qt::WindowSystemMenuHint|
                                          Qt::WindowStaysOnTopHint));
    void setWindowIcon(QIcon);
    void setCentralWidget(QWidget *);

public slots:
    void changingTitle(QString);

protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void moveEvent(QMoveEvent *event);

private:
    YGTitleBar *titleBar;
    QVBoxLayout *mainLayout;
    //这个widget仅仅用来占位。将被替代。
    QWidget *centerWidget;
};

#endif // YGMAINWINDOW_H
