#include "word.h"

Word::Word(QWidget *parent, const QString &fileName)
    :QTextEdit(parent), Document(Document::DOC_WORD, fileName)
{
    show();
}

Word::~Word()
{

}

bool Word::load()
{
    return true;
}

bool Word::save()
{
    return true;
}
