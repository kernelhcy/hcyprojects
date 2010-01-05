#include <QtGui/QApplication>
#include "tipwindow.h"
#include <QDialog>
#include "gotocelldialog.h"
#include "sortdialog.h"
#include "iconeditor.h"
#include <QImage>
#include "plotter.h"
#include <QVector>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    TipWindow *tip = new TipWindow();
//    tip -> show();
//    tip -> move(0,100);
//    tip -> move(100, 200);


//    GoToCellDialog *dialog = new GoToCellDialog;
//    dialog->show();
//
//    SortDialog *sortdialog = new SortDialog;
//    sortdialog->setColumnRange('C', 'F');
//    sortdialog->show();

    QMainWindow win;
//    QImage image("a.png");
//    IconEditor ie;
//    ie.setIconImage(image);
    Plotter plotter;

    QVector<QPointF> data;
    for (int i = 0; i < 50; ++i)
    {
        data.append(QPointF(i , rand() % 10));
    }
    plotter.setCurveData(1, data);

    QVector<QPointF> data1;
    for (int i = 0; i < 50; ++i)
    {
        data1.append(QPointF(i , rand() % 10));
    }
    plotter.setCurveData(2, data1);

    QVector<QPointF> data2;
    for (int i = 0; i < 50; ++i)
    {
        data2.append(QPointF(i , rand() % 10));
    }
    plotter.setCurveData(3, data2);

    QVector<QPointF> data3;
    for (int i = 0; i < 50; ++i)
    {
        data3.append(QPointF(i , rand() % 10));
    }
    plotter.setCurveData(4, data3);

    QVector<QPointF> data4;
    for (int i = 0; i < 50; ++i)
    {
        data4.append(QPointF(i , rand() % 10));
    }
    plotter.setCurveData(5, data4);

    QVector<QPointF> data5;
    for (int i = 0; i < 50; ++i)
    {
        data5.append(QPointF(i , rand() % 10));
    }
    plotter.setCurveData(6, data5);

    win.setCentralWidget(&plotter);
    win.show();
    return a.exec();

}
