package mt_tcp.server;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.SocketException;


public class TransferThreadServer implements Runnable
  {

    /**
     *
     * @param con
     * @param fileName
     * @param startPos
     * @param len
     */
    public TransferThreadServer(Socket con, String fileName, long startPos, long len)
      {
        this.connetcionSocket = con;
        this.fileName = fileName;
        this.startPos = startPos;
        this.transferLength = len;

        System.out.println("开始位置：" + startPos + "\t输出长度：" + transferLength);

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

            File file = new File(fileName);

            dis = new BufferedInputStream(new FileInputStream(file));
            //从指定开始输出数据
            dis.skip(startPos);

            System.out.println("输出数据...");
            /*
             * 向客户端发送文件数据
             */
            int len = 0;// 读取到缓冲区中的字节数
            len = dis.read(buffer);
            long totalLen = 0;

            if (transferLength < BUFFERSIZE)
              {
                outToClient.write(buffer, 0, len < transferLength ? len : (int) transferLength);
                outToClient.flush();
                totalLen = transferLength;
                len = -1;//不进入循环
              }
            totalLen += len;
            while (len != -1 && totalLen <= transferLength)
              {
                /**
                 * 必须按照缓冲区中实际的数据量写入流中，不可用outTOClient.write(result);简单地
                 * 将缓冲区统统写到流中，否则会报connection reset异常！！！
                 */
                outToClient.write(buffer, 0, len);
                len = dis.read(buffer);
                if (len < 0)
                  {
                    break;
                  }
                totalLen += len;
                Thread.sleep(10);
              }
            /*
             *	最后一次可能的传送，传送的数据小于BUFFERSIZE
             */
            if (totalLen < transferLength)
              {
                len = dis.read(buffer, 0, (int) (transferLength - totalLen));
                if (len > 0)
                  {
                    outToClient.write(buffer, 0, len);
                  }

              }
            outToClient.flush();
            System.out.println("输出文件长度：" + totalLen);
          } catch (InterruptedException ex)
          {
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
                dis.close();
                inFromClient.close();
                outToClient.close();
                connetcionSocket.close();
              } catch (IOException e)
              {
                // TODO Auto-generated catch block
                e.printStackTrace();
              }
          }

      }
    /*
     * *******************************
     *        声明成员变量
     * *******************************
     */
    // 缓存大小
    private static final int BUFFERSIZE = 1024;
    // 客户端发送的数据
    private String fileName = null;
    //开始传送的位置
    private long startPos = -1;
    //传送长度
    private long transferLength = -1;

    // 返回给客户端的数据
    private byte[] buffer = null;
    // 连接socket
    private Socket connetcionSocket = null;
    // 输入数据流
    private BufferedInputStream inFromClient = null;
    // 输出数据流
    private BufferedOutputStream outToClient = null;
    //
    private BufferedInputStream dis = null;
  }
