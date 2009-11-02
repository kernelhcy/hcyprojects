#include "gotocelldialog.h"

GoToCellDialog::GoToCellDialog(QWidget *parent)
        :QDialog(parent)
{
    QVBoxLayout *vbox = new QVBoxLayout(this);

    QHBoxLayout *hbox1 = new QHBoxLayout();
    line_label = new QLabel(tr("Line      :"));
    line_edit = new HexSpinBox();

    hbox1->addWidget(line_label);
    hbox1->addWidget(line_edit);

    vbox->addLayout(hbox1);

    QHBoxLayout *hbox2 = new QHBoxLayout();
    col_label = new QLabel(tr("Column:"));
    col_edit = new HexSpinBox();

    hbox2->addWidget(col_label);
    hbox2->addWidget(col_edit);

    vbox->addLayout(hbox2);

    ok = new QPushButton(tr("确定"));
    cancel = new QPushButton(tr("取消"));
    connect(ok, SIGNAL(clicked()), this, SLOT(okAction()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelAction()));

    QHBoxLayout *hbox3 = new QHBoxLayout();

    hbox3 -> addWidget(ok);
    hbox3 -> addWidget(cancel);

    vbox -> addLayout(hbox3);
    show();
    setFixedSize(size());
}

GoToCellDialog::~GoToCellDialog()
{
    delete line_label;
    delete line_edit;
    delete col_label;
    delete col_edit;
    delete ok;
    delete cancel;

}

void GoToCellDialog::okAction()
{

    this -> close();
}

void GoToCellDialog::cancelAction()
{
    this -> close();
}
