#ifndef GOTOCELLDIALOG_H
#define GOTOCELLDIALOG_H

#include <QtGui>
#include <QDialog>
#include "hexspinbox.h"
class HexSpinBox;
class QLabel;
class QPushButton;

class GoToCellDialog : public QDialog
{
    Q_OBJECT

public:
    GoToCellDialog(QWidget *parent = 0);
    ~GoToCellDialog();
    int line;
    int column;
private:
    HexSpinBox *col_edit, *line_edit;
    QLabel *col_label, * line_label;
    QPushButton *ok, *cancel;
private slots:
    void okAction();
    void cancelAction();
};

#endif // GOTOCELLDIALOG_H
