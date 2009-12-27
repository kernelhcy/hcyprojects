#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QString>

/*
 * 定义文档类型的父类型。
 * 分为四中文档：
 *  PPT(讲稿)，SS(电子表格)，WORD(文档)和UNKNOWN。
 * 默认初始化为UNKNOWN。
 *
 * 父类中提供文档类型信息，便于进行父类型到子类型的转换。
 * 设定通用的操作，子类覆盖。
 */
class Document
{

public:
    //定义文档四种类型。
    typedef enum
    {
        DOC_PPT,
        DOC_SS,
        DOC_WORD,
        DOC_UNKNOEN //默认为这种类型。
    }DOC_Type;

    Document()
        :type(DOC_UNKNOEN),fileName("")
    {
    };
    Document(DOC_Type type_, const QString &file)
        :type(type_), fileName(file)
    {
    };

    virtual ~Document()
    {};

    //获取文档的类型。
    DOC_Type getType() const
    {
        return type;
    }

    void setFileName(const QString &file)
    {
        fileName = file;
    }

    //载入和保存文档。
    virtual bool load() = 0;
    virtual bool save() = 0;

protected:

    //文档的类型又子类在构造函数中进行设定，且一但类型设定以后，将不再可以改变。
    //显然，一个对象的类型不可能发生改变。除了向其父类转变。

    //设置文档类型。
    void setType(DOC_Type t)
    {
        type = t;
    }

    //文件类型
    DOC_Type type;
    //文档名
    QString fileName;

};

#endif // DOCUMENT_H
