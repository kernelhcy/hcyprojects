package mt_tcp.server;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Calendar;

public class MainAcceptThread
{

    /**
     * 设置监听的端口号
     * 
     * @param port
     */
    public MainAcceptThread(int port)
    {
        this.port = port;
        try
        {
            // 创建serversocket
			/*
             * 设置最大并发连接个数
             */
            serverSocket = new ServerSocket(this.port, this.maxconnecion);

            /*
             * 设置连接超时。不限时。
             */
            serverSocket.setSoTimeout(0);

        } catch (IOException e)
        {
        }
    }

    public void acceptConnection()
    {
        if (serverSocket == null)
        {
            return;
        }
        //System.out.println("aaa"+logger);

        if (serverGui != null)
        {
            serverGui.writeInfo("Begin accept...");
            serverGui.writeInfo("Time:" + Calendar.getInstance().getTime().toString());
            serverGui.writeInfo("\n");
        }


        while (!isshutdown)
        {

            try
            {
                Socket tempconn = serverSocket.accept();
                inFromClient = new BufferedInputStream(tempconn.getInputStream());
                outToClient = new BufferedOutputStream(tempconn.getOutputStream());


                buffer = new byte[BUFFERSIZE];
                int len = inFromClient.read(buffer);
                input = new String(buffer, 0, len);

                if ("GetFileLength".equals(input))
                {
                    if (serverGui != null)
                    {
                        serverGui.writeInfo("\nAction: GetFileLength \n");
                        serverGui.writeInfo("Time:" + Calendar.getInstance().getTime().toString());
                        serverGui.writeInfo("\nIP:" + tempconn.getInetAddress().getHostAddress());
                        serverGui.writeInfo(" Port:" + tempconn.getPort());
                        serverGui.writeInfo("\n");
                    }


                    //返回客户端确认信息
                    outToClient.write("done".getBytes());
                    outToClient.flush();

                    // 创建新的线程处理请求
                    FileLengthFeedBackThread action = new FileLengthFeedBackThread(tempconn);
                    Thread newThread = new Thread(action);
                    //设置为守护线程
                    //newThread.setDaemon(true);
                    newThread.start();
                } else if ("Transfer".equals(input))
                {
                    if (serverGui != null)
                    {
                        serverGui.writeInfo("\nAction: Transfer \n");
                        serverGui.writeInfo("Time:" + Calendar.getInstance().getTime().toString());
                        serverGui.writeInfo("\nIP:" + tempconn.getInetAddress().getHostAddress());
                        serverGui.writeInfo(" Port:" + tempconn.getPort());
                        serverGui.writeInfo("\n");
                    }


                    //返回客户端确认信息
                    outToClient.write("done".getBytes());
                    outToClient.flush();

                    //获取传送的文件的信息
                    String name, lenStr, startPosStr;
                    long transferLength, startPos;
                    //获取文件名
                    len = inFromClient.read(buffer);
                    name = new String(buffer, 0, len);
                    outToClient.write("done".getBytes());
                    outToClient.flush();

                    //获取传送的开始位置
                    len = inFromClient.read(buffer);
                    startPosStr = new String(buffer, 0, len);
                    outToClient.write("done".getBytes());
                    outToClient.flush();
                    startPos = Long.parseLong(startPosStr);

                    //获取传送长度
                    len = inFromClient.read(buffer);
                    lenStr = new String(buffer, 0, len);
                    outToClient.write("done".getBytes());
                    outToClient.flush();
                    transferLength = Long.parseLong(lenStr);

                    // 创建新的线程处理请求
                    TransferThreadServer action = new TransferThreadServer(tempconn, name, startPos, transferLength);
                    Thread newThread = new Thread(action);
                    //设置为守护线程
                    //newThread.setDaemon(true);
                    newThread.start();
                } else if ("Filelist".equals(input))
                {
                    System.out.println("Action: Get Filelist ");

                    if (serverGui != null)
                    {
                        serverGui.writeInfo("\nAction: Get Filelist \n");
                        serverGui.writeInfo("Time:" + Calendar.getInstance().getTime().toString());
                        serverGui.writeInfo("\nIP:" + tempconn.getInetAddress().getHostAddress());
                        serverGui.writeInfo(" Port:" + tempconn.getPort());
                        serverGui.writeInfo("\n");
                    }


                    //返回客户端确认信息
                    //outToClient.write("done".getBytes());
                    //outToClient.flush();

                    // 创建新的线程处理请求
                    SendBackFileList action = new SendBackFileList(tempconn);
                    Thread newThread = new Thread(action);
                    //设置为守护线程
                    //newThread.setDaemon(true);
                    newThread.start();
                }

            } catch (IOException e)
            {
            }

            if (isshutdown)
            {
                try
                {
                    serverSocket.close();



                    if (serverGui != null)
                    {
                        serverGui.writeInfo("\nClosing serversocket.\n");
                    }
                } catch (IOException e)
                {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }

        }

        if (!serverSocket.isClosed())
        {
            try
            {
                serverSocket.close();

            } catch (IOException e)
            {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
        return;
    }

    /**
     * 关闭服务器
     */
    public void stop()
    {
        isshutdown = true;
        if (!serverSocket.isClosed())
        {
            try
            {
                serverSocket.close();


            } catch (IOException e)
            {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
    }

    public ServerGUI getServerGui()
    {
        return serverGui;
    }

    public void setServerGui(ServerGUI serverGui)
    {
        this.serverGui = serverGui;
    }
    /*
     * ******************************* 
     *        声明成员变量
     * *******************************
     */
    // 服务socket
    private ServerSocket serverSocket = null;
    // socket监听的端口号
    private int port;
    //最大并发连接个数
    private int maxconnecion = 50;
    private boolean isshutdown = false;
    //用于向界面输出信息
    private ServerGUI serverGui = null;
    // 缓存大小
    private static final int BUFFERSIZE = 1024;
    // 客户端发送的数据
    private String input = null;
    // 返回给客户端的数据
    private byte[] buffer = null;
    // 输入数据流
    private BufferedInputStream inFromClient = null;
    // 输出数据流
    private BufferedOutputStream outToClient = null;
}
