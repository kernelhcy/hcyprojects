#ifndef RULEDIALOG_H
#define RULEDIALOG_H

#include <QDialog>
#include <QSize>
#include <QLabel>
#include <QHBoxLayout>

/**
  可以使用QMessageBox代替。
 */
class RuleDialog : public QDialog
{
public:
    RuleDialog(QWidget *parent = 0);
};

#endif // RULEDIALOG_H
