#include "gotocelldialog.h"
#include "ui_gotocelldialog.h"

GoToCellDialog::GoToCellDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::GoToCellDialog)
{
    m_ui->setupUi(this);
}

GoToCellDialog::~GoToCellDialog()
{
    delete m_ui;
}

void GoToCellDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
