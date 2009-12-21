#include <QtGui/QApplication>
#include "tipwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    TipWindow *tip = new TipWindow();
    tip -> show();
    tip -> move(0,100);
    tip -> move(100, 200);
    return a.exec();

}
