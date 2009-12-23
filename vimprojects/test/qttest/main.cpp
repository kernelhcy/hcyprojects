#include <QtGui/QApplication>
#include "tipwindow.h"
#include <QDialog>
#include "gotocelldialog.h"
#include "sortdialog.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    TipWindow *tip = new TipWindow();
//    tip -> show();
//    tip -> move(0,100);
//    tip -> move(100, 200);


    GoToCellDialog *dialog = new GoToCellDialog;
    dialog->show();

    SortDialog *sortdialog = new SortDialog;
    sortdialog->setColumnRange('C', 'F');
    sortdialog->show();

    return a.exec();

}
