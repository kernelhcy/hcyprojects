#include "spreadsheet.h"
#include <QtGui>

Spreadsheet::Spreadsheet(QWidget *parent, const QString &fileName)
    :QTableWidget(parent), Document(Document::DOC_SS, fileName)
{
    autoRecalc = true;
    rowCount = 50;
    columnCount = 60;
    setItemPrototype(new Cell);
    setSelectionMode(ContiguousSelection);
    connect(this, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(somethingChanged()));
    clear();
    setMinimumSize(sizeHint());
    //setWindowTitle(fileName + tr("[*]"));
}
Spreadsheet::~Spreadsheet()
{

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
        return c -> formula();
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
    //setWindowModified(true);
    emit modified();
}

void Spreadsheet::recalculate()
{
    return;
}

void Spreadsheet::findNext(const QString &str, Qt::CaseSensitivity cs)
{
    int row = currentRow();
    int column = currentColumn() + 1;
    while (row < rowCount)
    {
        while (column < columnCount)
        {
            if (text(row, column).contains(str, cs))
            {
                clearSelection();
                setCurrentCell(row, column);
                activateWindow();
                return;
            }
            ++column;
        }
        column = 0;
        ++row;
    }
    QApplication::beep();

}
void Spreadsheet::findPrevious(const QString &str, Qt::CaseSensitivity cs)
{
    QTextStream out(stdout);
    out << "Find Previous: " << str;
    switch (cs)
    {
    case Qt::CaseInsensitive:
        out << " Case Insensitive.\n";
        break;
    case Qt::CaseSensitive:
        out << " Case Sensitive.\n";
        break;
    default:
        out << " ERROR! Unknown case sensitivity.\n";
        break;
    }
}

void Spreadsheet::setCurrentCell(int row, int column)
{
    QTextStream out(stdout);
    out << "Spreadsheet set current cell to ( "<< row << "," << column <<" ).\n";
    return;
}

void Spreadsheet::sort(const SpreadsheetCompare &compare)
{
    QTextStream out(stdout);
    out << "SpreadSheet sort: " << "\n";
    return;
}
QTableWidgetSelectionRange Spreadsheet::selectedRange() const
{
    QList<QTableWidgetSelectionRange> ranges = QTableWidget::selectedRanges();
    if (ranges.isEmpty())
        return QTableWidgetSelectionRange();
    return ranges.first();
}

bool Spreadsheet::save()
{
    if (!writeFile(fileName))
    {
        return false;
    }
    return true;
}

bool Spreadsheet::load()
{
    if (!readFile(fileName))
    {
        return false;
    }

    return true;
}

bool Spreadsheet::writeFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Spreadsheet"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(file.fileName())
                             .arg(file.errorString()));
        return false;
    }
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_4_1);
    out << quint32(MagicNumber);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    for (int row = 0; row < rowCount; ++row) {
        for (int column = 0; column < columnCount; ++column) {
            QString str = formula(row, column);
            if (!str.isEmpty())
                out << quint16(row) << quint16(column) << str;
        }
    }
    QApplication::restoreOverrideCursor();
    return true;
}

bool Spreadsheet::readFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Spreadsheet"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(file.fileName())
                             .arg(file.errorString()));
        return false;
    }
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_4_1);
    quint32 magic;
    in >> magic;
    if (magic != MagicNumber) {
        QMessageBox::warning(this, tr("Spreadsheet"),
                             tr("The file is not a Spreadsheet file."));
        return false;
    }
    clear();
    quint16 row;
    quint16 column;
    QString str;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    while (!in.atEnd()) {
        in >> row >> column >> str;
        setFormula(row, column, str);
    }
    QApplication::restoreOverrideCursor();
    return true;
}

void Spreadsheet::copy()
{
    QTableWidgetSelectionRange range = selectedRange();
    QString str;
    for (int i = 0; i < range.rowCount(); ++i)
    {
        if (i > 0)
            str += "\n";
        for (int j = 0; j < range.columnCount(); ++j)
        {
            if (j > 0)
                str += "\t";
            str += formula(range.topRow() + i, range.leftColumn() + j);
        }
    }
    QApplication::clipboard()->setText(str);
}

void Spreadsheet::paste()
{
    QTableWidgetSelectionRange range = selectedRange();
    QString str = QApplication::clipboard()->text();
    QStringList rows = str.split('\n');
    int numRows = rows.count();
    int numColumns = rows.first().count('\t') + 1;
    if (range.rowCount() * range.columnCount() != 1
            && (range.rowCount() != numRows
                || range.columnCount() != numColumns))
    {
        QMessageBox::information(this, tr("Spreadsheet"),
                tr("The information cannot be pasted because the copy "
                   "and paste areas aren't the same size."));
        return;
    }

    for (int i = 0; i < numRows; ++i)
    {
        QStringList columns = rows[i].split('\t');
        for (int j = 0; j < numColumns; ++j)
        {
            int row = range.topRow() + i;
            int column = range.leftColumn() + j;
            if (row < rowCount && column < columnCount)
                setFormula(row, column, columns[j]);
        }
    }
    somethingChanged();
}

void Spreadsheet::del()
{
    foreach (QTableWidgetItem *item, selectedItems())
        delete item;
}

void Spreadsheet::cut()
{
    copy();
    del();
}


//QTableWidget's selectRow() and selectColumn() functions
void Spreadsheet::selectCurrentRow()
{
    selectRow(currentRow());
}

void Spreadsheet::selectCurrentColumn()
{
    selectColumn(currentColumn());
}


