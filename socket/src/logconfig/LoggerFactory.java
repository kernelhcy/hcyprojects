/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package logconfig;

import java.io.IOException;
import java.util.Calendar;
import java.util.logging.ConsoleHandler;
import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;

/**
 * logger工厂
 * @author hcy
 */
public class LoggerFactory
{
    //私有构造函数，防止使用new构造多个实例。

    private LoggerFactory()
    {
        throw new IllegalAccessError();
    }

    /*
     * 获得一个Logger实例
     */
    public static Logger getInstance(String className)
    {
        return createInstance(className);

    }

    /*
     * 创建一个Logger实例。
     */
    private static Logger createInstance(String className)
    {
        try
        {
            logger = Logger.getLogger(className);

            //%g为自动编号
            FileHandler fh = new FileHandler("log.log", true);
            fh.setFormatter(new MyFormatter());
            fh.setLevel(Level.ALL);
            logger.addHandler(fh);

            //ConsoleHandler ch = new ConsoleHandler();
            //logger.addHandler(ch);

            logger.setLevel(Level.ALL);

            
        } catch (IOException ex)
        {
            System.out.println(ex.getMessage());
        } catch (SecurityException ex)
        {
            System.out.println(ex.getMessage());
        }
        return logger;
    }

    /*
     *  日志的输出格式
     */
    private static class MyFormatter extends Formatter
    {

        public String format(LogRecord logRecord)
        {
            StringBuilder sb = new StringBuilder("LogRecord info: ");
            sb.append(logRecord.getSourceClassName()); //就是哪个类里面用的了你
            sb.append("\n");
            sb.append(Calendar.getInstance().toString());
            sb.append("\t\t");
            sb.append(logRecord.getLevel()); //是SEVERE还是WARNING还是别的
            sb.append("\t\t");
            sb.append(logRecord.getLoggerName());//这个logger发布者
            sb.append("\t\t");
            sb.append(logRecord.getMessage());//消息内容
            sb.append("\t\n\n");

            return sb.toString();
        }
    }

    private static Logger logger = null;
}
