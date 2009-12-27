#ifndef WORD_H
#define WORD_H

#include "document.h"
#include <QTextEdit>

class Word : public QTextEdit, public Document
{
public:
    Word(QWidget *parent = 0, const QString &fileName = "");
    ~Word();

    bool load();
    bool save();
private:

};

#endif // WORD_H
