#include "aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent)
        :QDialog(parent)
{
    resize(QSize(170,150));
    setWindowTitle(tr("关于BreakOut"));
    show();
}
