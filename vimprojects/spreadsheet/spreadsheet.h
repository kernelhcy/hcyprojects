#ifndef SPREADSHEET_H
#define SPREADSHEET_H

#include <QtGui>
#include <QTableWidget>
#include "cell.h"
#include "heads.h"

class Cell;
class SpreadsheetCompare;

class Spreadsheet : public QTableWidget
{
    Q_OBJECT

    public:
        Spreadsheet(QWidget *parent = 0);
        //内联函数
        bool autoRecalculate()const
        {
            return autoRecalc;
        }
        QString currentLocation() const;
        QTableWidgetSelectionRange selectedRange() const;
        void clear();
        bool readFile(const QString &fileName);
        bool writeFile(const QString &fileName);
        void sort(const SpreadsheetCompare &compare);
        QString currentFormula() const;
    public slots:
        //void cut();
        //void copy();
        //void paste();
        //void del();
        //void selectCurrentRow();
        //void selectCurrentColumn();
        void recalculate();
        //void setAutoRecalculate(bool recalc);
        //void findNext(const QString &str, Qt::CaseSensitivity cs);
        //void findPrevioue(const QString &str, Qt::CaseSensitivity cs);
    signals:
        void modified();
    private slots:
        void somethingChanged();
    private:
        enum
        {
            MagicNumber = 0x7F51C883,
        };
        Cell* cell(int row, int column)const;
        QString text(int row, int column)const;
        QString formula(int row, int column)const;
        QString getVIndex(int index);
        void setFormula(int row, int column, const QString &formula);
        bool autoRecalc;
        int rowCount;
        int columnCount;
};

class SpreadsheetCompare
{
public :
    bool operator ()( const QStringList &row1, const QStringList &row2)const;
    enum { KeyCount = 3 };
    int keys[KeyCount];
    bool ascending[KeyCount];
};

#endif
