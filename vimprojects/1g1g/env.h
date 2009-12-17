#ifndef ENV_H
#define ENV_H

/**
 * 程序的一些环境变量
 */

namespace ENV1G
{
  //定义窗口的显示模式。
    typedef enum
    {
        V_NORMAL,   //正常模式
        V_SIMPLE,   //精简模式
        V_LISTEN    //听歌模式

    }viewMode;
}

#endif // ENV_H
