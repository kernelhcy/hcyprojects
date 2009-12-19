#include "tipwindow.h"

TipWindow::TipWindow(QWidget *parent)
        :QWidget(0, Qt::FramelessWindowHint)
{

    this -> resize(2, 50);
    this -> parent = parent;

    QDesktopWidget *desktop = QApplication::desktop();
    screenWidth = desktop->width();
    screenHeight = desktop->height();

    QSize size = this -> size();
    QPoint pos;

    pos.setX(this -> screenWidth);
    //将窗口放在0.618黄金分割点的位置
    pos.setY((int)(this -> screenHeight * 0.382));

    this -> move(pos);

}

TipWindow::~TipWindow()
{

}

void TipWindow::setPosition(TipWinPos pos)
{
    this -> pos = pos;
    return;
}

//在显示和隐藏窗口时，体现窗口滑动的效果。
 void TipWindow::pollOut()
 {

//     pos = position;
//     while(position.x() > this -> screenWidth - size.width())
//     {
//         pos.setX(pos.x() - 10);
//         this->move(pos);
//         sleep(1);
//         printf("Move Window %d\n" , pos.x());
//         this -> show();
//     }
     this->resize(220, 50);
     printf("Poll Out.\n");
 }

 void TipWindow::pollIn()
 {

 }

 void TipWindow::moveEvent(QMoveEvent *event)
 {
     this -> position = event -> pos();
     return;
 }
