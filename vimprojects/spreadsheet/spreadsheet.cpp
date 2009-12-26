#include "spreadsheet.h"

Spreadsheet::Spreadsheet(QWidget *parent)
        :QTableWidget(parent)
{
    autoRecalc = true;
    rowCount = 150;
    columnCount = 2500;
    setItemPrototype(new Cell);
    setSelectionMode(ContiguousSelection);
    connect(this, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(somethingChanged()));
    clear();
    setMinimumSize(sizeHint());
}

void Spreadsheet::clear()
{
    setColumnCount(0);
    setRowCount(0);
    setColumnCount(columnCount);
    setRowCount(rowCount);

    for  (int i = 0; i < columnCount; ++i)
    {
        QTableWidgetItem *item = new QTableWidgetItem();
        item -> setText(getVIndex(i));
        setHorizontalHeaderItem(i, item);
        setColumnWidth(i, 80);
    }

    for (int i = 0; i < rowCount; ++i)
    {
        QTableWidgetItem *item = new QTableWidgetItem();
        item -> setText(QString("%1").arg(i + 1));
        setVerticalHeaderItem(i, item);
    }

    for (int i = 0; i < rowCount; ++i)
    {
        for (int j = 0; j < columnCount; ++j)
        {
            QTableWidgetItem *item = new QTableWidgetItem();
            //item -> setText(QString("AAA"));
            setItem(i, j, item);
        }
    }

    setCurrentCell(0, 0);
}

QString Spreadsheet::getVIndex(int index)
{
    int val = index;

    if (val < 26)
    {
        return QString(QChar('A' + val));
    }

    int tmp = 26;
    QString s_index("");
    int s_len = 1;
    while (val > tmp)
    {
        val -= tmp;
        ++s_len;
        tmp *=26;
    }
    --val;

    while(s_len > 0)
    {
        --s_len;
        tmp = val % 26;
        s_index = s_index.insert(0, QString("%1").arg(QChar('A' + tmp)));
        val /= 26;
    }

    return s_index;
}

Cell *Spreadsheet::cell( int row, int column) const
{
    return static_cast<Cell *>(item(row, column));
}

QString Spreadsheet::text( int row, int column) const
{
    Cell *c = cell(row, column);
    if (c)
    {
        return c->text();
    }
    else
    {
        return "";
    }
}
QString Spreadsheet::formula( int row, int column) const
{
    Cell *c = cell(row, column);
    if (c)
    {
        return c->formula();
    }
    else
    {
        return "";
    }
}

void Spreadsheet::setFormula(int row, int column, const QString &formula)
{
    Cell *c = cell(row, column);
    if (!c)
    {
        c = new Cell();
        setItem(row, column, c);
    }

    c -> setFormula(formula);

}

QString Spreadsheet::currentLocation() const
{
    return QChar('A' + currentColumn()) + QString::number(currentRow() + 1);
}

QString Spreadsheet::currentFormula() const
{
    return formula(currentRow(), currentColumn());
}

void Spreadsheet::somethingChanged()
{
    if (autoRecalc)
        recalculate();
    emit modified();
}

void Spreadsheet::recalculate()
{
    return;
}
