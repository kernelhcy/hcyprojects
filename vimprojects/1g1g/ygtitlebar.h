#ifndef YGTITLEBAR_H
#define YGTITLEBAR_H

#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include <QIcon>

#include "ygmainwindow.h"

class YGMainWindow;

/**
 * 窗口的额标题拦
 * 包括一个图标，标题，最小化按钮和关闭按钮。
 * -----------------------------
 * |icon| title ....       |-|X|
 * -----------------------------
 */
class YGTitleBar : public QWidget
{
Q_OBJECT
public:
    YGTitleBar(QWidget *parent = 0);
    ~YGTitleBar();
    void setIcon(const QIcon&);

public slots:
    void setTitle(QString&);

protected slots:
    void closeWindow();
    void minimumWindow();

private:
    QLabel *title;
    QToolButton *icon;
    QWidget *parent;
};

#endif // YGTITLEBAR_H
