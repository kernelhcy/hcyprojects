#include "dialogtest.h"
#include "ui_dialogtest.h"

DialogTest::DialogTest(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::DialogTest)
{
    m_ui->setupUi(this);
}

DialogTest::~DialogTest()
{
    delete m_ui;
}

void DialogTest::changeEvent(QEvent *e)
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
