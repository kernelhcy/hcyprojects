#include "ygmainwindow.h"
#include "qwidgetresizehandler_p.h"
#include <QtGui>

YGMainWindow::YGMainWindow(QWidget *parent, Qt::WindowFlags flag) :
    QWidget(parent, flag)
{
    QWidgetResizeHandler *wrh = new QWidgetResizeHandler(this);
    wrh ->setActive(QWidgetResizeHandler::Resize, true);

    mainLayout = new QVBoxLayout(this);

    titleBar = new YGTitleBar(this);
    mainLayout -> addWidget(titleBar);

    centerWidget = new QWidget(this);
    centerWidget -> setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    mainLayout -> addWidget(centerWidget);

    mainLayout -> setSpacing(0);
    mainLayout -> setMargin(0);
    //setContentsMargins(3, 0, 3, 3);
    setLayout(mainLayout);
}

void YGMainWindow::setWindowIcon(QIcon i)
{
    QWidget::setWindowIcon(i);
    titleBar -> setIcon(i);
}

void YGMainWindow::setCentralWidget(QWidget *w)
{
    w -> setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    mainLayout -> deleteLater();
    mainLayout -> addWidget(w);
    w -> setParent(this);
}

void YGMainWindow::changingTitle(QString t)
{
    QWidget::setWindowTitle(t);
    titleBar -> setTitle(t);
}

void YGMainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void YGMainWindow::moveEvent(QMoveEvent *event)
{
    Q_UNUSED(event);
}
