#include "ruledialog.h"

RuleDialog::RuleDialog(QWidget *parent)
        :QDialog(parent)
{
    resize(QSize(170,150));
    setWindowTitle(tr("游戏规则"));
    show();
}
