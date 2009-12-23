#ifndef GOTOCELLDIALOG_H
#define GOTOCELLDIALOG_H

#include <QtGui/QDialog>

namespace Ui
{
    class GoToCellDialog;
}

class GoToCellDialog : public QDialog
{
    Q_OBJECT
public:
    GoToCellDialog(QWidget *parent = 0);
    ~GoToCellDialog();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::GoToCellDialog *m_ui;
};

#endif // GOTOCELLDIALOG_H
