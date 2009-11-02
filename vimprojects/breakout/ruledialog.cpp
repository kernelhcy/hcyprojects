#include "ruledialog.h"

RuleDialog::RuleDialog(QWidget *parent)
        :QDialog(parent)
{
    resize(QSize(170,150));
    setWindowTitle(tr("游戏规则"));
    QString rules = tr("<h2><center>游戏规则</center></h2>"
                    "<p>1.空格键开始游戏。"
                    "<p>2.p键暂停游戏。"
                    "<p>3.左右方向键控制滑块的移动方向。"
                    "<p>4.Esc键退出游戏。"
                    "<p>5.游戏有两种模式：随机模式和正常模式。"
                    "<p>正常模式:球的反射是规律的，并且速度恒定。"
                    "<p>随机模式:球的移动速度和反射方向是随机的。"
                    "<p>6.可以设置游戏的速度和砖块的行列数。"
                    );
    QLabel *rule_label = new QLabel(rules);
    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox -> addWidget(rule_label);
    show();
}
