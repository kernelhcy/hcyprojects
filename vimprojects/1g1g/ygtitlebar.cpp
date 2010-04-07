#include "ygtitlebar.h"
#include <QtGui>

YGTitleBar::YGTitleBar(QWidget *p) :
    QWidget(p), parent(p)
{

    QHBoxLayout *layout = new QHBoxLayout();
    setLayout(layout);

    icon = new QToolButton(this);
    icon  -> setFixedSize(20, 20);
    title = new QLabel("",this);
    title -> setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    title -> resize(title -> width(), 20);

    QPushButton *btnMinimize = new QPushButton("v");
    btnMinimize -> setFixedSize(20, 20);
    connect(btnMinimize, SIGNAL(released ()), this, SLOT(minimumWindow()));
    QPushButton *btnClose = new QPushButton("X");
    btnClose -> setFixedSize(20, 20);
    connect(btnClose, SIGNAL(released ()), this, SLOT(closeWindow()));

    layout -> addWidget(icon);
    layout -> addWidget(title);
    layout -> addWidget(btnMinimize);
    layout -> addWidget(btnClose);

    layout -> setSpacing(0);
    layout -> setContentsMargins(0, 0, 0, 0);

    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    //QMetaObject::connectSlotsByName(this);
}

YGTitleBar::~YGTitleBar()
{

}

void YGTitleBar::setIcon(const QIcon &i)
{
    icon -> setIcon(i);
}

void YGTitleBar::setTitle(QString &t)
{
    title -> setText(t);
}
void YGTitleBar::closeWindow()
{
    parent -> close();
}
void YGTitleBar::minimumWindow()
{
    parent -> hide();
}
