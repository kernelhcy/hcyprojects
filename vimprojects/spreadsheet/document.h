#ifndef DOCUMENT_H
#define DOCUMENT_H
/*
 * 定义文档类型的父类型。
 * 分为四中文档：
 *  PPT(讲稿)，SS(电子表格)，WORD(文档)和UNKNOWN。
 * 默认初始化为UNKNOWN。
 *
 * 父类中提供文档类型信息，便于进行父类型到子类型的转换。
 *
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

    Document():type(DOC_UNKNOEN){};
    Document(DOC_Type type_):type(type_){};

    //获取文档的类型。
    DOC_Type getType() const
    {
        return type;
    }

protected:

    //文档的类型又子类在构造函数中进行设定，且一但类型设定以后，将不再可以改变。
    //显然，一个对象的类型不可能发生改变。除了向其父类转变。

    //设置文档类型。
    void setType(DOC_Type t)
    {
        type = t;
    }

    DOC_Type type;

};

#endif // DOCUMENT_H
