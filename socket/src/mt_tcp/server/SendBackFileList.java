package mt_tcp.server;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.IOException;
import java.net.Socket;
import java.net.SocketException;

/**
 * 用于返回客户端其请求的文件的长度。
 *
 * @author hcy
 *
 */
public class SendBackFileList implements Runnable
{

    /**
     *
     * @param conn
     */
    public SendBackFileList(Socket conn)
    {
        this.connetcionSocket = conn;
        logger = MyLogger.getInstance();
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

            logger.info("SendBack File list : receive dir");

            int len = -1;//
            len = inFromClient.read(buffer);
            dirName = new String(buffer, 0, len);

            logger.info("Dir: " + dirName);

            File dir = new File(dirName);

            if (dir.isDirectory())
            {
                logger.info("get the file names");

                fileAndDirNames = dir.list();

                StringBuilder sb = new StringBuilder();
                for (String s : fileAndDirNames)
                {
                    sb.append(s);
                    sb.append(',');
                }

                names = sb.toString();
            }
            else // not directory
            {
                names = "Not a directory!";
            }



            logger.info("Send file list back.");

            outToClient.write(names.getBytes());
            outToClient.flush();

            logger.info("Send file list Over!");

        } catch (SocketException e)
        {
            // TODO Auto-generated catch block
            e.printStackTrace();

        } catch (IOException e)
        {
            // TODO Auto-generated catch block
            e.printStackTrace();

        } finally
        {
            try
            {
                // 关闭流和socket
                inFromClient.close();
                outToClient.close();
                connetcionSocket.close();
                logger.info("SendBackFileList : Close connection.");
            } catch (IOException e)
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
    //日志
    private MyLogger logger = null;
}
