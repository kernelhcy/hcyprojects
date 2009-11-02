#ifndef CELL_H
#define CELL_H

#include <QTableWidgetItem>
#include "heads.h"

class Cell : public QTableWidgetItem
{
public:
    Cell();
    void setFormula(const QString &formula);
    QString formula();
};

#endif // CELL_H
