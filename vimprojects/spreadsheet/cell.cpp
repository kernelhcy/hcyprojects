#include "cell.h"
#include <QtGui>
Cell::Cell()
{
    setDirty();
}

QTableWidgetItem *Cell::clone() const
{
    return new Cell(*this);
}

void Cell::setFormula(const QString &formula)
{
    setData(Qt::EditRole, formula);
}


QString Cell::formula() const
{
    return data(Qt::EditRole).toString();
}

void Cell::setData(int role, const QVariant &value)
{
    QTableWidgetItem::setData(role, value);
    if (role == Qt::EditRole)
        setDirty();
}

void Cell::setDirty()
{
    cacheIsDirty = true;
}

QVariant Cell::data(int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (value().isValid())
        {
            return value().toString();
        }
        else
        {
            return "####";
        }
    }
    else if (role == Qt::TextAlignmentRole)
    {
        if (value().type() == QVariant::String)
        {
            return int(Qt::AlignLeft | Qt::AlignVCenter);
        }
        else
        {
            return int(Qt::AlignRight | Qt::AlignVCenter);
        }
    }
    else
    {
        return QTableWidgetItem::data(role);
    }
}

const QVariant Invalid;
/*
 * Return the value of the this cell.
 * If the formula starts with "'", return the string of the formula which from the 1 position.
 * If the formula starts with "=", evaluate the result of this formula and return the
 * value.
 * If the formula starts with none of the upper two, we will try convert the formulate to
 * a double value. If the convertion successed, we will return the double value, or return
 * the formula.
 *
 * All of the result will be packaged with QVariant.
 */
QVariant Cell::value() const
{
    if (cacheIsDirty)
    {
        cacheIsDirty = false;
        QString formulaStr = formula();
        if (formulaStr.startsWith('\'')) //start with '
        {
            cachedValue = formulaStr.mid(1);
        }
        else if (formulaStr.startsWith('=')) //run this formula
        {
            cachedValue = Invalid;
            QString expr = formulaStr.mid(1);
            expr.replace(" ", "");
            expr.append(QChar::Null);
            int pos = 0;
            cachedValue = evalExpression(expr, pos);
            if (expr[pos] != QChar::Null)
                cachedValue = Invalid;
        }
        else
        {
            bool ok;
            double d = formulaStr.toDouble(&ok);
            if (ok)
            {
                cachedValue = d;
            }
            else
            {
                cachedValue = formulaStr;
            }
        }
    }
    return cachedValue;
}

/*
 * The last three functions--evalExpression,evalTerm and evalFactor are used to
 * run the formula.
 * In our program, the formula is an expression. An expression is consist of +, - and
 * terms.An term is consist of *, / and factors. An Factor is consist of number, cell
 * position, (expression) and -(minus).
 *
 * The Sytanx Diagram For Cell Expression:
 *
 *        Expression                      Term
 *         --------                     ---------
 *    --->|  Term  |--->           --->|  Factor |--->
 *      ^  --------   |              ^  ---------   |
 *      |   -----     |              |    -----     |
 *      ---|  +  |<---|              --- |  *  |<---|
 *      ^   -----     |              ^    -----     |
 *      |   -----     |              |    -----     |
 *      ---|  -  |<---|              ----|  /  |<---|
 *          -----                         -----
 *
 *
 *        Factor
 *                                  --------
 *    ---------------------------> | Number | ---------------------->
 *       |         ^  |             --------                   ^
 *       |   ---   |  |          ---------------               |
 *       -->| - |---  |-------> | Cell Location | ------------>|
 *           ---      |          ---------------               |
 *                    |    ---       ------------       ---    |
 *                    |-> | ( | --> | Expression | --> | ) | -->
 *                         ---       ------------       ---
 */

/* The parameter pos is a refernce, be careful! */
QVariant Cell::evalExpression(const QString &str, int &pos) const
{
    QVariant result = evalTerm(str, pos);
    while (str[pos] != QChar::Null)
    {
        QChar op = str[pos];
        if (op != '+' && op != '-')
            return result;
        ++pos;
        QVariant term = evalTerm(str, pos);
        if (result.type() == QVariant::Double && term.type() == QVariant::Double)
        {
            if (op == '+')
            {
                result = result.toDouble() + term.toDouble();
            }
            else
            {
                result = result.toDouble() - term.toDouble();
            }
        }
        else
        {
            result = Invalid;
        }
    }
    return result;
}

QVariant Cell::evalTerm(const QString &str, int &pos) const
{
    QVariant result = evalFactor(str, pos);
    while (str[pos] != QChar::Null)
    {
        QChar op = str[pos];
        if (op != '*' && op != '/')
        {
            return result;
        }
        ++pos;
        QVariant factor = evalFactor(str, pos);
        if (result.type() == QVariant::Double && factor.type() == QVariant::Double)
        {
            if (op == '*')
            {
                result = result.toDouble() * factor.toDouble();
            }
            else
            {
                if (factor.toDouble() == 0.0)
                {
                    result = Invalid;
                }
                else
                {
                    result = result.toDouble() / factor.toDouble();
                }
            }
        }
        else
        {
            result = Invalid;
        }
    }
    return result;
}

QVariant Cell::evalFactor(const QString &str, int &pos) const
{
    QVariant result;
    bool negative = false;
    if (str[pos] == '-')
    {
        negative = true;
        ++pos;
    }
    if (str[pos] == '(')
    {
        ++pos;
        result = evalExpression(str, pos);
        if (str[pos] != ')')
            result = Invalid;
        ++pos;
    }
    else
    {
        QRegExp regExp("[A-Za-z][1-9][0-9]{0,2}");
        QString token;
        while (str[pos].isLetterOrNumber() || str[pos] == '.')
        {
            token += str[pos];
            ++pos;
        }
        if (regExp.exactMatch(token))
        {
            int column = token[0].toUpper().unicode() - 'A';
            int row = token.mid(1).toInt() - 1;
            Cell *c = static_cast<Cell *>(tableWidget()->item(row, column));
            if (c)
            {
                result = c->value();
            }
            else
            {
                result = 0.0;
            }
        }
        else
        {
            bool ok;
            result = token.toDouble(&ok);
            if (!ok)
                result = Invalid;
        }
    }
    if (negative)
    {
        if (result.type() == QVariant::Double)
        {
            result = -result.toDouble();
        }
        else
        {
            result = Invalid;
        }
    }
    return result;
}


