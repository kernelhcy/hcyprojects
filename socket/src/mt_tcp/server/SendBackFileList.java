package mt_tcp.server;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.IOException;
import java.net.Socket;
import java.net.SocketException;
import java.util.logging.Logger;
import logconfig.LoggerFactory;

/**
 * 用于返回客户端其请求的文件的长度。
 *
 * @author hcy
 *
 */
public class SendBackFileList implements Runnable
{
    //初始化日志

    static
    {
        logger =
                LoggerFactory.getInstance(SendBackFileList.class.getName());
    }

    /**
     *
     * @param conn
     */
    public SendBackFileList(Socket conn)
    {
        this.connetcionSocket = conn;
    }

    /**
     *
     */
    @Override
    public void run()
    {
        /*
         * 设置连接超时。不限时。
         */
        try
        {
            buffer = new byte[BUFFERSIZE];

            connetcionSocket.setSoTimeout(0);

            // 获得输入输出流
            inFromClient = new BufferedInputStream(connetcionSocket.getInputStream());
            outToClient = new BufferedOutputStream(connetcionSocket.getOutputStream());

            //logger.info("SendBack File list : receive dir");

            int len = -1;//
            len = inFromClient.read(buffer);
            dirName = new String(buffer, 0, len);

            File dir = new File(dirName);

            if (!dir.isDirectory())
            {
            }

            //logger.info("get the file names");

            fileAndDirNames = dir.list();

            StringBuilder sb = new StringBuilder();
            for (String s : fileAndDirNames)
            {
                sb.append(s);
                sb.append(',');
            }

            names = sb.toString();
            //logger.info(names);

            outToClient.write(names.getBytes());
            outToClient.flush();

        }
        catch (SocketException e)
        {
            // TODO Auto-generated catch block
            e.printStackTrace();
            logger.severe(e.getMessage());
        }
        catch (IOException e)
        {
            // TODO Auto-generated catch block
            e.printStackTrace();
            logger.severe(e.getMessage());
        }
        finally
        {
            try
            {
                // 关闭流和socket
                inFromClient.close();
                outToClient.close();
                connetcionSocket.close();
            }
            catch (IOException e)
            {
                // TODO Auto-generated catch block
                e.printStackTrace();
                //logger.severe(e.getMessage());
            }
        }

    }
    /*
     * *******************************
     *        声明成员变量
     * *******************************
     */
    //
    private static Logger logger = null;
    // 缓存大小
    private static final int BUFFERSIZE = 1024;
    //
    private String[] fileAndDirNames = null;
    private String names = null;
    // 目录
    private String dirName = null;
    // 返回给客户端的数据
    private byte[] buffer = null;
    // 连接socket
    private Socket connetcionSocket = null;
    // 输入数据流
    private BufferedInputStream inFromClient = null;
    // 输出数据流
    private BufferedOutputStream outToClient = null;
}
