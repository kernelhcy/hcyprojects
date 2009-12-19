#ifndef TIPWINDOW_H
#define TIPWINDOW_H

#include <QWidget>
#include <QtGui>

/*
 * 无边框窗口
 * 用于显示当前切换歌曲。窗口实现从屏幕边缘的拖出和推回
 */

class TipWindow : public QWidget
{
public:
    TipWindow(QWidget *parent = 0);
    ~TipWindow();

    //用于设定窗口的位置
    typedef enum
    {
        TW_UP,      //窗口在上边显示
        TW_DOWN,    //下边
        TW_LEFT,    //左边
        TW_RIGHT    //右边
    }TipWinPos;

    //设置窗口的位置
    void setPosition(TipWinPos pos);

    //从屏幕边缘拉出和推回窗口
    void pollOut();
    void pollIn();

protected:
    void moveEvent(QMoveEvent *event);

private:
    QWidget *parent;
    int screenWidth, screenHeight;
    TipWinPos pos;
    QPoint position;//窗口的当前位置坐标。

};

#endif // TIPWINDOW_H
