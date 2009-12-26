#ifndef SPREADSHEET_H
#define SPREADSHEET_H

#include <QTableWidget>
#include "cell.h"
#include "heads.h"
#include "document.h"

class SpreadsheetCompare;

/*
 * Spreadsheet继承自QTableWidget,是整个程序的核心控件。
 * 负责处理表格。
 * 表格的每一个单元格都是一个Cell对象。Cell继承自QTableWidgetItem，用来存放每个单元格中的数据。
 *
 */
class Spreadsheet : public QTableWidget, public Document
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
        /*
         * 清空表格，并将其设置为所设定的行数和列数（rowCount,columnCount）
         */
        void clear();


        bool readFile(const QString &fileName);
        bool writeFile(const QString &fileName);

        void sort(const SpreadsheetCompare &compare);
        QString currentFormula() const;

        void setCurrentCell(int row, int column);

    public slots:
        //void cut();
        //void copy();
        //void paste();
        //void del();
        //void selectCurrentRow();
        //void selectCurrentColumn();
        void recalculate();
        //void setAutoRecalculate(bool recalc);
        void findNext(const QString &str, Qt::CaseSensitivity cs);
        void findPrevious(const QString &str, Qt::CaseSensitivity cs);
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
        /*
         * 将列号（index）转换成由字母计数的形式。
         * 如：
         *          0   ->  A
         *          2   ->  C
         *          25  ->  Z
         *          26  ->  AA
         *          52  ->  AZ
         *          53  ->  BA  等等。
         */
        QString getVIndex(int index);

        /*
         * 设置（row，column）单元格的计算公式为formula。
         */
        void setFormula(int row, int column, const QString &formula);

        //是否自动计算公式的结果。
        bool autoRecalc;
        //表格当前的行数和列数
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
