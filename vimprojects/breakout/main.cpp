#include "breakout.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Breakout bo;

    bo.resize(300, 400);
    bo.setWindowTitle("Breakout");
    bo.show();

    return app.exec();
}
