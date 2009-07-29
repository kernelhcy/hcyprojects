/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package mt_tcp.server;

import java.util.Calendar;
import java.util.logging.Logger;
import logconfig.LoggerFactory;

/**
 *
 * @author hcy
 */
public class MyLogger
{
    /**
      * 单例模式
      * 只有一个日志类的实例存在。
      */
    private MyLogger()
    {
        
    }

    /**
     * 获得日志类的实例
     * @return
     */
    public static MyLogger getInstance()
    {
        if(logger == null)
        {
            logger = new MyLogger();
        }

        return logger;
    }

    /**
     *  输出日志信息
     * @param s
     */
    synchronized public void info(String s)
    {
        Calendar now = Calendar.getInstance();
        System.out.print("MyLogger: ");
        System.out.println(now.getTime().toString() + " : " + s);
    }

    //实例
    private static MyLogger logger = null;
}
