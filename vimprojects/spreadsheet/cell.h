#ifndef CELL_H
#define CELL_H

#include <QTableWidgetItem>
#include "heads.h"

class Cell : public QTableWidgetItem
{
public:
    Cell();
    /*
     * QTableWidget uses this function to create a new cell.
     * The instance passed to QTableWidget::setItemPrototype() is the item that is cloned.
     * Use prototype partener
     */
    QTableWidgetItem *clone() const;

    /*
     * Every QTableWidgetItem can hold some data, up to one QVariant for each data "role".
     * The most commonly used roles are Qt::EditRole and Qt::DisplayRole.
     * The edit role is used for data that is to be edited, and the display role is for
     * data that is to be displayed. Often the data for both is the same,
     * but in Cell the edit role corresponds to the cell's formula and the display role
     * corresponds to the cell's value (the result of evaluating the formula).
     */
    void setData(int role, const QVariant &value);
    QVariant data(int role) const;
    void setFormula(const QString &formula);
    QString formula() const;

    /*
     * This function just sets the cacheIsDirty to true and forces a recalculation of
     * the cell's value.
     */
    void setDirty();

private:
    QVariant value() const;

    /*
     * These three functions are used to calculate the value of the cell's formula.
     */
    QVariant evalExpression(const QString &str, int &pos) const;
    QVariant evalTerm(const QString &str, int &pos) const;
    QVariant evalFactor(const QString &str, int &pos) const;

    //cache the cell's value
    mutable QVariant cachedValue;
    //If the cachedValue is not up to date, this value is true.
    mutable bool cacheIsDirty;
};

#endif // CELL_H
